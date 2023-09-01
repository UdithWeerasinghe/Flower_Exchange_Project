#include <iostream>
#include <vector>
#include <queue>
#include <fstream>
#include <sstream>
#include <string>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <algorithm>
#include <map>
using namespace std;

class Item {
    string item_class_order_id, item_instrument;
    int item_side, item_quantity;
    double item_price;

public:
    Item(string item_class_order_id, string item_instrument, int item_side, int item_quantity, double item_price) {
        
}

};

string getTime() {
    auto now = std::chrono::system_clock::now();
    std::time_t now_time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
    std::tm* tm_struct = std::localtime(&now_time_t);
    char time_str[20];
    std::strftime(time_str, sizeof(time_str), "%Y%m%d-%H%M%S", tm_struct);
    std::ostringstream oss;
    oss << time_str << "." << std::setw(3) << std::setfill('0') << ms.count();
    return oss.str();
}

struct BuyOrderBook {
    int buy_order_side;
    string buy_order_id;
    double buy_order_price;
    int buy_order_quantity;
    int buy_order_received_id;

    BuyOrderBook(int received_id, int side, string id, double price, int quantity) {
        buy_order_side = side;
        buy_order_id = id;
        buy_order_price = price;
        buy_order_quantity = quantity;
        buy_order_received_id = received_id;
    }

    bool operator<(BuyOrderBook obj) const {
        return (buy_order_price > obj.buy_order_price);
    }
};

struct SellOrderBook {
    int sell_order_side;
    string sell_order_id;
    double sell_order_price;
    int sell_order_quantity;
    int sell_order_received_id;

    SellOrderBook(int received_id, int side, string id, double price, int quantity) {
        sell_order_side = side;
        sell_order_id = id;
        sell_order_price = price;
        sell_order_quantity = quantity;
        sell_order_received_id = received_id;
    }

    
    bool operator<(SellOrderBook obj) const {
        return (sell_order_price < obj.sell_order_price);
    }
};

class TradeOrder {
    string order_client_order_id;
    string order_order_id;
    string order_instrument;
    int order_side;
    double order_price;
    int order_quantity;
    int order_status;
    string order_reason;
    string order_transaction_time;

    TradeOrder() {
    }
};

int main() {
    ifstream inputFile;
    inputFile.open("C:\\Users\\Udith\\OneDrive\\Desktop\\LSEG\\order.csv");
    ofstream outputFile;
    outputFile.open("C:\\Users\\Udith\\OneDrive\\Desktop\\LSEG\\execution_rep1.csv");
    outputFile << "client_order_id , " << "order_id , " << "instrument , " << "side , " << "price , " << "quantity , " << "status , " << "reason , " << "transaction time " << endl;

    string line = "";

    vector<BuyOrderBook> buyOrderBook;
    vector<SellOrderBook> sellOrderBook;

    std::map<std::string, std::vector<BuyOrderBook>> buyOrderBooks;
    std::map<std::string, std::vector<SellOrderBook>> sellOrderBooks;

    int itemCount = -1;

    while (getline(inputFile, line)) {
        if (itemCount < 0) {
            itemCount++;
            continue;
        }
        itemCount++;
        string item_class_order_id;
        string item_instrument;
        int item_side;
        int item_quantity;
        double item_price;
        string tempString;
        stringstream inputString(line);

        getline(inputString, item_class_order_id, ',');
        getline(inputString, item_instrument, ',');
        getline(inputString, tempString, ',');
        item_side = atoi(tempString.c_str());
        getline(inputString, tempString, ',');
        item_quantity = atoi(tempString.c_str());
        getline(inputString, tempString, ',');
        item_price = stod(tempString.c_str());
        string reason;
        bool is_valid = true;

        if (item_quantity > 1000 || item_quantity < 10) {
            reason = "Invalid quantity";
            is_valid = false;
        } else if (item_quantity % 10 != 0) {
            reason = "Invalid quantity";
            is_valid = false;
        } else if (item_price <= 0) {
            reason = "Invalid price";
            is_valid = false;
        } else if (item_instrument != "Rose" && item_instrument != "Lavender" && item_instrument != "Tulip" && item_instrument != "Orchid" && item_instrument != "Lotus") {
            reason = "Invalid instrument";
            is_valid = false;
        } else if (item_side != 1 && item_side != 2) {
            reason = "Invalid side";
            is_valid = false;
        }
        if (!is_valid) {
            outputFile << "Order" << itemCount << ", " << item_class_order_id << " ," << item_instrument << " ," << item_side << ", " << item_price << ", " << item_quantity << " ," << "Reject" << ", " << reason << " ," << getTime() << endl;
            continue;
        } else {
            if (buyOrderBooks.find(item_instrument) != buyOrderBooks.end()) {
                buyOrderBook = buyOrderBooks[item_instrument];
                sellOrderBook = sellOrderBooks[item_instrument];
            }
            vector<BuyOrderBook> buyBook;
            vector<SellOrderBook> sellBook;

            if (item_side == 1) {
                BuyOrderBook book(itemCount, item_side, item_class_order_id, item_price, item_quantity);
                buyOrderBook.push_back(book);
                sort(buyOrderBook.begin(), buyOrderBook.end());
                bool buyingHappen = true;

                if (sellOrderBook.size() == 0) {
                    outputFile << "ORD202300" << itemCount << ", " << item_class_order_id << " ," << item_instrument << " ," << item_side << ", " << item_price << " ," << item_quantity << ", " << "New" << " ," << "N/A" << " ," << getTime() << endl;
                }

                while (buyingHappen) {
                    if (buyOrderBook.size() > 0 && sellOrderBook.size() > 0) {
                        buyBook = buyOrderBook;
                        sellBook = sellOrderBook;
                        if (buyBook[0].buy_order_price < sellBook[0].sell_order_price) {
                            buyingHappen = false;
                            outputFile << "ORD202300" << itemCount << " ," << item_class_order_id << " ," << item_instrument << " ," << item_side << ", " << item_price << " ," << item_quantity << ", " << "New" << " ," << "N/A" << " ," << getTime() << endl;
                        } else if (buyBook[0].buy_order_quantity <= sellBook[0].sell_order_quantity) {
                            outputFile << "ORD202300" << itemCount << " ," << item_class_order_id << " ," << item_instrument << " ," << item_side << ", " << item_price << ", " << item_quantity << ", " << "Fill" << " ," << "N/A" << " ," << getTime() << endl;
                            sellBook[0].sell_order_quantity -= buyBook[0].buy_order_quantity;
                            if (sellBook[0].sell_order_quantity == 0) {
                                outputFile << "ORD202300" << sellBook[0].sell_order_received_id << " ," << sellBook[0].sell_order_id << ", " << item_instrument << " ," << item_side << ", " << item_price << ", " << sellBook[0].sell_order_quantity << " ," << "Fill" << " ," << "N/A" << " ," << getTime() << endl;
                                sellBook.erase(sellBook.begin());
                            }
                            buyBook.erase(buyBook.begin());
                            buyingHappen = false;
                        } else {
                            outputFile << "ORD202300" << sellBook[0].sell_order_received_id << sellBook[0].sell_order_id << " ," << item_instrument << " ," << item_side << " ," << item_price << ", " << sellBook[0].sell_order_quantity << ", " << "Fill" << ", " << "N/A" << ", " << getTime() << endl;
                            buyBook[0].buy_order_quantity -= sellBook[0].sell_order_quantity;
                            item_quantity = buyBook[0].buy_order_quantity;
                            sellBook.erase(sellBook.begin());
                            outputFile << "ORD202300" << itemCount << " ," << item_class_order_id << " ," << item_instrument << " ," << item_side << " ," << item_price << " ," << item_quantity << " ," << "Pfill" << " ,"  << "N/A" << " ," << getTime() << endl;
                        }
                        buyOrderBook = buyBook;
                        sellOrderBook = sellBook;
                    } else {
                        buyingHappen = false;
                    }
                }

            } else if (item_side == 2) {
                SellOrderBook book(itemCount, item_side, item_class_order_id, item_price, item_quantity);
                sellOrderBook.push_back(book);
                sort(sellOrderBook.begin(), sellOrderBook.end());
                bool sellingHappen = true;

                if (buyOrderBook.size() == 0) {
                    outputFile << "ORD202300" << itemCount << ", " << item_class_order_id << " ," << item_instrument << " ," << item_side << ", " << item_price << " ," << item_quantity << ", " << "New" << " ," << "N/A" << " ," << getTime() << endl;
                }

                while (sellingHappen) {

                    if (buyOrderBook.size() > 0 && sellOrderBook.size() > 0) {

                        buyBook = buyOrderBook;
                        sellBook = sellOrderBook;

                        if (buyBook[0].buy_order_price < sellBook[0].sell_order_price || buyBook.size() == 0) {
                            sellingHappen = false;
                            outputFile << "ORD202300" << itemCount << ", " << item_class_order_id << " ," << item_instrument << " ," << item_side << " ," << item_price << " ," << item_quantity << " ," << "New" << " ," << "N/A" << " ," << getTime() << endl;
                        } else if (buyBook[0].buy_order_quantity >= sellBook[0].sell_order_quantity) {
                            outputFile << "ORD202300" << itemCount << ", " << item_class_order_id << " ," << item_instrument << " ," << item_side << ", " << item_price << ", " << item_quantity << " ," << "Fill" << " ," << "N/A" << " ," << getTime() << endl;
                            buyBook[0].buy_order_quantity -= sellBook[0].sell_order_quantity;
                            if (buyBook[0].buy_order_quantity == 0) {
                                outputFile << "ORD202300" << buyBook[0].buy_order_received_id << ", " << buyBook[0].buy_order_id << " ," << item_instrument << ", " << item_side << " ," << item_price << " ," << buyBook[0].buy_order_quantity << " ," << "Fill" << " ," << "N/A" << " ," << getTime() << endl;
                                buyBook.erase(buyBook.begin());
                            }
                            sellBook.erase(sellBook.begin());
                            sellingHappen = false;
                        } else {
                            outputFile << "ORD202300" << buyBook[0].buy_order_received_id << ", " << buyBook[0].buy_order_id << " ," << item_instrument << ", " << item_side << " ," << item_price << " ," << buyBook[0].buy_order_quantity << " ," << "Fill" << " ," << "N/A" << " ," << getTime() << endl;
                            sellBook[0].sell_order_quantity -= buyBook[0].buy_order_quantity;
                            item_quantity = sellBook[0].sell_order_quantity;
                            buyBook.erase(buyBook.begin());
                            outputFile << "ORD202300" << itemCount << " ," << item_class_order_id << " ," << item_instrument << ", " << item_side << " ," << item_price << " ," << item_quantity << " ," << "Pfill" << " ," << "N/A" << " ," << getTime() << endl;
                        }
                        buyOrderBook = buyBook;
                        sellOrderBook = sellBook;
                    } else {
                        sellingHappen = false;
                    }
                }
            }

            buyOrderBooks[item_instrument] = buyOrderBook;
            sellOrderBooks[item_instrument] = sellOrderBook;
        }
    }
    inputFile.close();
    outputFile.close();
}

