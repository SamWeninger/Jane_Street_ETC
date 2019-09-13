/* Base code for getting messages from the market exchange provided by Jane Street */
/* 
 * Author:    Samuel Weninger
 * Created:   June 8th, 2019
 * Modified:  September 12th, 2019   (minor commenting)
 *
**/

/* C includes for networking things */
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>

/* C++ includes */
#include <string>
#include <iostream>
#include <stdexcept>
#include <algorithm>
#include <vector>
#include <sstream>

#include <chrono>
#include <fstream>

/* The Configuration class is used to tell the bot how to connect
 to the appropriate exchange. The `test_exchange_index` variable
 only changes the Configuration when `test_mode` is set to `true`.
 */
class Configuration {
private:
    /*
     0 = prod-like
     1 = slower
     2 = empty
     */
    static int const test_exchange_index = 1;
public:
    std::string team_name;
    std::string exchange_hostname;
    int exchange_port;

    // team name = 'STOCKOVERFLOW'
    Configuration(bool test_mode) : team_name("STOCKOVERFLOW"){
        exchange_port = 20000; /* Default text based port */
        if(true == test_mode) {
            exchange_hostname = "test-exch-" + team_name;
            exchange_port += test_exchange_index;
        } else {
            exchange_hostname = "production";
        }
    }
};

/* Connection establishes a read/write connection to the exchange,
 and facilitates communication with it */
class Connection {
private:
    FILE * in;
    FILE * out;
    int socket_fd;
public:
    Connection(Configuration configuration){
        int sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0) {
            throw std::runtime_error("Could not create socket");
        }
        std::string hostname = configuration.exchange_hostname;
        hostent *record = gethostbyname(hostname.c_str());
        if(!record) {
            throw std::invalid_argument("Could not resolve host '" + hostname + "'");
        }
        in_addr *address = reinterpret_cast<in_addr *>(record->h_addr);
        std::string ip_address = inet_ntoa(*address);
        struct sockaddr_in server;
        server.sin_addr.s_addr = inet_addr(ip_address.c_str());
        server.sin_family = AF_INET;
        server.sin_port = htons(configuration.exchange_port);
        
        int res = connect(sock, ((struct sockaddr *) &server), sizeof(server));
        if (res < 0) {
            throw std::runtime_error("could not connect");
        }
        FILE *exchange_in = fdopen(sock, "r");
        if (exchange_in == NULL){
            throw std::runtime_error("could not open socket for writing");
        }
        FILE *exchange_out = fdopen(sock, "w");
        if (exchange_out == NULL){
            throw std::runtime_error("could not open socket for reading");
        }
        
        setlinebuf(exchange_in);
        setlinebuf(exchange_out);
        this->in = exchange_in;
        this->out = exchange_out;
        this->socket_fd = res;
    }
    
    /** Send a string to the server */
    void send_to_exchange(std::string input) {
        std::string line(input);
        /* All messages must always be uppercase */
        std::transform(line.begin(), line.end(), line.begin(), ::toupper);
        int res = fprintf(this->out, "%s\n", line.c_str());
        if (res < 0) {
            throw std::runtime_error("error sending to exchange");
        }
    }
    
    /** Read a line from the server, dropping the newline at the end */
    std::string read_from_exchange()
    {
        /* We assume that no message from the exchange is longer
         than 10,000 chars */
        const size_t len = 10000;
        char buf[len];
        if(!fgets(buf, len, this->in)){
            throw std::runtime_error("reading line from socket");
        }
        
        int read_length = strlen(buf);
        std::string result(buf);
        /* Chop off the newline */
        result.resize(result.length() - 1);
        return result;
    }
};

/** Join a vector of strings together, with a separator in-between
 each string. This is useful for space-separating things */
std::string join(std::string sep, std::vector<std::string> strs) {
    std::ostringstream stream;
    const int size = strs.size();
    for(int i = 0; i < size; ++i) {
        stream << strs[i];
        if(i != (strs.size() - 1)) {
            stream << sep;
        }
    }
    return stream.str();
}


/* Struct to hold details of financial instruments (stocks, bonds, ETFs, ADRs, etc.) */
struct stock_data {
    int time_id;
    int price;
    int volume;
};

int main(int argc, char *argv[])
{
    bool test_mode = false;
    Configuration config(test_mode);
    Connection conn(config);
    
    /* Communication to market */
    std::vector<std::string> data;
    data.push_back(std::string("HELLO"));
    data.push_back(config.team_name);

    /* Strings to hold information from market */
    std::string send;
    std::string price;
    std::string quantity;
    
    conn.send_to_exchange(join(" ", data));
    
    /* Initial check for communication status to market */
    //conn.send_to_exchange(std::string("HELLO STOCKOVERFLOW\n"));
    
    /* See information presented by market */
    /*for (int i = 0; i < 200; i++){
        std::cout << "number" << i << ":";
        std::cout << conn.read_from_exchange() << std::endl;
    }*/
    
    /* Check communication to server */
    //conn.send_to_exchange(std::string("HELLO STOCKOVERFLOW\n"));
    //std::cout << conn.read_from_exchange() << std::endl;

    /* Iterator to check through vector data for financial items */
    int j = 1;
    
    /* Initialization of vectors and structures to hold information about financial instruments */
    
    /* Vectors of 'stock_data' structs */
    std::vector<stock_data> MS;
    std::vector<stock_data> BOND;
    std::vector<stock_data> VALBZ;
    std::vector<stock_data> VALE;
    std::vector<stock_data> GS;
    std::vector<stock_data> WFC;
    std::vector<stock_data> XLF;
    
    /* Struct initialization */
    stock_data MS_struct;
    stock_data BOND_struct;
    stock_data VALBZ_struct;
    stock_data VALE_struct;
    stock_data GS_struct;
    stock_data WFC_struct;
    stock_data XLF_struct;
    
    int temp_time = 0; // Temporary time variable used to calculate elapsed time
    
    auto start = std::chrono::steady_clock::now(); // Save starting time
    
    while (temp_time < 300) { // Only run while within each 'day' (5 min) of simulated market
        /* Read from the market exchange, and read in information about each item */
        std::string line = conn.read_from_exchange();
        std::stringstream lineStream(line); // Create stream to read in word at a time from exchange output
        std::string trade;
        std::string symbol;
        std::string price;
        std::string volume;
        
        /* Identify trade status */
        lineStream >> trade;
        if (trade == "TRADE") {
            lineStream >> symbol;
            /* Identify the item symbol */
            
            if (symbol == "MS") {
                lineStream >> price;
                lineStream >> volume;
                
                /* Save exchange information into struct for specific item */
                MS_struct.volume += std::stoi(volume);
                MS_struct.price = std::stoi(price);
                
            }
            else if (symbol == "BOND") {
                lineStream >> price;
                lineStream >> volume;
                
                /* Save exchange information into struct for specific item */
                BOND_struct.volume += std::stoi(volume);
                BOND_struct.price = std::stoi(price);
                
            }
            else if (symbol == "VALBZ") {
                lineStream >> price;
                lineStream >> volume;
                
                /* Save exchange information into struct for specific item */
                VALBZ_struct.volume += std::stoi(volume);
                VALBZ_struct.price = std::stoi(price);
                
            }
            else if (symbol == "VALE") {
                lineStream >> price;
                lineStream >> volume;
                
                /* Save exchange information into struct for specific item */
                VALE_struct.volume += std::stoi(volume);
                VALE_struct.price = std::stoi(price);
                
            }
            else if (symbol == "GS") {
                lineStream >> price;
                lineStream >> volume;
                
                /* Save exchange information into struct for specific item */
                GS_struct.volume += std::stoi(volume);
                GS_struct.price = std::stoi(price);
                
            }
            else if (symbol == "WFC") {
                lineStream >> price;
                lineStream >> volume;
                
                /* Save exchange information into struct for specific item */
                WFC_struct.volume += std::stoi(volume);
                WFC_struct.price = std::stoi(price);
                
            }
            else if (symbol == "XLF") {
                lineStream >> price;
                lineStream >> volume;
                
                /* Save exchange information into struct for specific item */
                XLF_struct.volume += std::stoi(volume);
                XLF_struct.price = std::stoi(price);
                
            }
        }
        
        /* Check for elapsed time from start */
        auto end = std::chrono::steady_clock::now();
        int time = std::chrono::duration_cast<std::chrono::seconds>(end - start).count();
        
        if (time != temp_time) {
            /* Send time to output */
            std::cout << time << std::endl;
            
            /* Store struct data read from exchange into vector for respective item */
            
            MS_struct.time_id = temp_time;
            MS.emplace_back(MS_struct);
            MS_struct.time_id = 0;
            MS_struct.volume = 0;
            
            BOND_struct.time_id = temp_time;
            BOND.emplace_back(BOND_struct);
            BOND_struct.time_id = 0;
            BOND_struct.volume = 0;
            
            VALBZ_struct.time_id = temp_time;
            VALBZ.emplace_back(VALBZ_struct);
            VALBZ_struct.time_id = 0;
            VALBZ_struct.volume = 0;
            
            VALE_struct.time_id = temp_time;
            VALE.emplace_back(VALE_struct);
            VALE_struct.time_id = 0;
            VALE_struct.volume = 0;
            
            GS_struct.time_id = temp_time;
            GS.emplace_back(GS_struct);
            GS_struct.time_id = 0;
            GS_struct.volume = 0;
            
            WFC_struct.time_id = temp_time;
            WFC.emplace_back(WFC_struct);
            WFC_struct.time_id = 0;
            WFC_struct.volume = 0;
            
            XLF_struct.time_id = temp_time;
            XLF.emplace_back(XLF_struct);
            XLF_struct.time_id = 0;
            XLF_struct.volume = 0;
            
            
            /* General economic strategy for buying and selling market items */
            
            if (VALE_struct.price > VALBZ_struct.price + 10) {
                conn.send_to_exchange("ADD " + std::to_string(j) + " VALE SELL " + std::to_string(VALE_struct.price) + " 2\n");
                conn.send_to_exchange("ADD " + std::to_string(j + 1) + " VALBZ BUY " + std::to_string(VALBZ_struct.price) + " 2\n");
            }
            
            else if (VALE_struct.price + 10 < VALBZ_struct.price) {
                conn.send_to_exchange("ADD " + std::to_string(j) + " VALBZ SELL " + std::to_string(VALBZ_struct.price) + " 2\n");
                conn.send_to_exchange("ADD " + std::to_string(j + 1) + " VALE BUY " + std::to_string(VALE_struct.price) + " 2\n");
            }
            
            if ((10 * XLF_struct.price) > (3 * BOND_struct.price) + (3 * MS_struct.price) + (2 * GS_struct.price) + (2 * WFC_struct.price) + 100) {
                conn.send_to_exchange("ADD " + std::to_string(j) + " XLF SELL " + std::to_string(XLF_struct.price) + " 2\n");
                
                /* Decided to focus on just the XLF items at a point in competition, so the below was commented */
                //conn.send_to_exchange("ADD " + std::to_string(j + 1) + " BOND BUY " + std::to_string(VALBZ_struct.price) + " 1\n");
            }
            
            else if ((10 * XLF_struct.price) + 100 < (3 * BOND_struct.price) + (3 * MS_struct.price) + (2 * GS_struct.price) + (2 * WFC_struct.price)) {
                conn.send_to_exchange("ADD " + std::to_string(j) + " XLF BUY " + std::to_string(XLF_struct.price) + " 2\n");
                
                /* Decided to focus on just the XLF items at a point in competition, so the below was commented */
                //conn.send_to_exchange("ADD " + std::to_string(j + 1) + " VALBZ BUY " + std::to_string(VALBZ_struct.price) + " 1\n");
            }
            
        }
        
        temp_time = time; // update temp time
        
        j+=2; // increment through vector
    }
    
    /* Output market information for further analysis in a .csv file */
    std::ofstream myfile;
    myfile.open ("stocks.csv");
    myfile << "\n\nMS\ntime,price,volume\n";
    for (auto idx : MS) {
        myfile << idx.time_id << "," << idx.price << "," << idx.volume << "\n";
    }
    
    myfile << "\n\nBOND\ntime,price,volume\n";
    for (auto idx : BOND) {
        myfile << idx.time_id << "," << idx.price << "," << idx.volume << "\n";
    }
    
    myfile << "\n\nVALBZ\ntime,price,volume\n";
    for (auto idx : VALBZ) {
        myfile << idx.time_id << "," << idx.price << "," << idx.volume << "\n";
    }
    
    myfile << "\n\nVALE\ntime,price,volume\n";
    for (auto idx : VALE) {
        myfile << idx.time_id << "," << idx.price << "," << idx.volume << "\n";
    }
    
    myfile << "\n\nGS\ntime,price,volume\n";
    for (auto idx : GS) {
        myfile << idx.time_id << "," << idx.price << "," << idx.volume << "\n";
    }
    
    myfile << "\n\nWFC\ntime,price,volume\n";
    for (auto idx : WFC) {
        myfile << idx.time_id << "," << idx.price << "," << idx.volume << "\n";
    }
    
    myfile << "\n\nXLF\ntime,price,volume\n";
    for (auto idx : XLF) {
        myfile << idx.time_id << "," << idx.price << "," << idx.volume << "\n";
    }
    
    myfile.close();
    
    
    std::cout << "The exchange replied: " << conn.read_from_exchange() << std::endl;
    
    return 0;
}

