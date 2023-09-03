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
#include <thread>  // Include the thread library

using namespace std;

struct OrderBookEntry {
    int side;
    string orderId;
    double price;
    int quantity;
    int receivedorderID;

    OrderBookEntry(int receivedorderID, int side, string orderId, double price, int quantity) {
        this->side = side;
        this->orderId = orderId;
        this->price = price;
        this->quantity = quantity;
        this->receivedorderID = receivedorderID;
    }

    bool operator<(const OrderBookEntry& obj) const {
        if (side == 1) {
            // For buy orders, sort by descending price
            return price > obj.price;
        } else {
            // For sell orders, sort by ascending price
            return price < obj.price;
        }
    }
};

map<string, pair<vector<OrderBookEntry>, vector<OrderBookEntry>>> orderBooks;

pair<vector<OrderBookEntry>, vector<OrderBookEntry>>& getOrderBook(const string& instrument) {
    return orderBooks[instrument];
}

string getCurrentTime() {
    auto currentTime = chrono::system_clock::now();
    auto currentTimeMillis = chrono::duration_cast<chrono::milliseconds>(currentTime.time_since_epoch()) % 1000;
    auto timeT = chrono::system_clock::to_time_t(currentTime);
    tm* timeInfo = localtime(&timeT);

    char timeStr[20];
    strftime(timeStr, sizeof(timeStr), "%Y%m%d-%H%M%S", timeInfo);

    stringstream formattedTime;
    formattedTime << timeStr << "." << setw(3) << setfill('0') << currentTimeMillis.count();
    return formattedTime.str();
}

void processOrderChunk(const string& inputFilePath, const string& outputFilePath, int startOrder, int endOrder) {
    ifstream inputFile(inputFilePath);
    ofstream outputFile(outputFilePath, ios_base::app);

    string line;
    int orderCount = -1;

    while (getline(inputFile, line)) {
        if (orderCount < 0) {
            orderCount++;
            continue;
        }
        orderCount++;

        if (orderCount >= startOrder && orderCount <= endOrder) {
            string orderId, instrument;
            int side, quantity;
            double price;
            string tempString;

            stringstream inputString(line);

            getline(inputString, orderId, ',');
            getline(inputString, instrument, ',');
            getline(inputString, tempString, ',');
            side = atoi(tempString.c_str());
            getline(inputString, tempString, ',');
            quantity = atoi(tempString.c_str());

            bool Valid = true;
            string reason;

            getline(inputString, tempString, ',');
            try {
                price = stod(tempString);
            } catch (const std::invalid_argument& e) {
                reason = "Invalid Price";
                Valid = false;
            }

            if (quantity > 1000 || quantity < 10) {
                reason = "Invalid Quantity";
                Valid = false;
            } else if (quantity % 10 != 0) {
                reason = "Invalid Quantity";
                Valid = false;
            } else if (!(price > 0)) {
                reason = "Invalid Price";
                Valid = false;
            } else if (instrument != "Rose" && instrument != "Lavender" && instrument != "Tulip" && instrument != "Orchid" && instrument != "Lotus") {
                reason = "Invalid Instrument";
                Valid = false;
            } else if (side != 1 && side != 2) {
                reason = "Invalid Side";
                Valid = false;
            }

            if (!Valid) {
                outputFile << "ORD2300" << orderCount << "," << orderId << "," << instrument << "," << side << "," << price << "," << quantity << "," << "Reject" << "," << reason << "," << getCurrentTime() << endl;
                continue;
            } else {
                pair<vector<OrderBookEntry>, vector<OrderBookEntry>>& currentOrderBook = getOrderBook(instrument);

                vector<OrderBookEntry>& buyOrderBook = currentOrderBook.first;
                vector<OrderBookEntry>& sellOrderBook = currentOrderBook.second;

                vector<OrderBookEntry> buyEntries;
                vector<OrderBookEntry> sellEntries;

                if (side == 1) {
                    OrderBookEntry bookEntry(orderCount, side, orderId, price, quantity);
                    buyOrderBook.push_back(bookEntry);
                    sort(buyOrderBook.begin(), buyOrderBook.end());

                    bool buyingHappen = true;
                    bool New = true;

                    if (sellOrderBook.size() == 0) {
                        outputFile << "ORD2300" << orderCount << "," << orderId << "," << instrument << "," << side << "," << price << "," << quantity << "," << "New" << "," << "," << getCurrentTime() << endl;
                        buyingHappen = false;
                    }

                    while (buyingHappen) {
                        if (sellOrderBook.size() > 0 && buyOrderBook.size() > 0) {
                            buyEntries = buyOrderBook;
                            sellEntries = sellOrderBook;

                            if (buyEntries[0].price < sellEntries[0].price) {
                                buyingHappen = false;
                                if (New) {
                                    outputFile << "ORD2300" << orderCount << "," << orderId << "," << instrument << "," << side << "," << price << "," << quantity << "," << "New" << "," << "," << getCurrentTime() << endl;
                                }
                            } else if (buyEntries[0].quantity <= sellEntries[0].quantity) {
                                outputFile << "ORD2300" << orderCount << "," << orderId << "," << instrument << "," << side << "," << sellEntries[0].price << "," << quantity << "," << "Fill" << "," << "," << getCurrentTime() << endl;

                                if (sellEntries[0].quantity == buyEntries[0].quantity) {
                                    outputFile << "ORD2300 " << sellEntries[0].receivedorderID << "," << sellEntries[0].orderId << "," << instrument << "," << sellEntries[0].side << "," << sellEntries[0].price << "," << buyEntries[0].quantity << "," << "Fill" << "," << "," << getCurrentTime() << endl;
                                    sellEntries.erase(sellEntries.begin());
                                } else {
                                    outputFile << "ORD2300 " << sellEntries[0].receivedorderID << "," << sellEntries[0].orderId << "," << instrument << "," << sellEntries[0].side << "," << sellEntries[0].price << "," << buyEntries[0].quantity << "," << "pFill" << "," << "," << getCurrentTime() << endl;
                                    sellEntries[0].quantity = sellEntries[0].quantity - buyEntries[0].quantity;
                                }
                                buyEntries.erase(buyEntries.begin());
                                buyingHappen = false;
                            } else {
                                outputFile << "ORD2300" << orderCount << "," << orderId << "," << instrument << "," << side << "," << sellEntries[0].price << "," << sellEntries[0].quantity << "," << "Pfill" << "," << "," << getCurrentTime() << endl;
                                outputFile << "ORD2300" << sellEntries[0].receivedorderID << "," << sellEntries[0].orderId << "," << instrument << "," << sellEntries[0].side << "," << sellEntries[0].price << "," << sellEntries[0].quantity << "," << "Fill" << "," << "," << getCurrentTime() << endl;

                                buyEntries[0].quantity = buyEntries[0].quantity - sellEntries[0].quantity;
                                quantity = buyEntries[0].quantity;
                                sellEntries.erase(sellEntries.begin());
                                New = false;
                            }
                            buyOrderBook = buyEntries;
                            sellOrderBook = sellEntries;
                        } else {
                            buyingHappen = false;
                        }
                    }
                } else if (side == 2) {
                    OrderBookEntry bookEntry(orderCount, side, orderId, price, quantity);
                    sellOrderBook.push_back(bookEntry);
                    sort(sellOrderBook.begin(), sellOrderBook.end());
                    bool sellingHappen = true;
                    bool New = true;

                    if (buyOrderBook.size() == 0) {
                        outputFile << "ORD2300" << orderCount << "," << orderId << "," << instrument << "," << side << "," << price << "," << quantity << "," << "New" << "," << "," << getCurrentTime() << endl;
                        sellingHappen = false;
                    }

                    while (sellingHappen) {
                        if (buyOrderBook.size() > 0 && sellOrderBook.size() > 0) {
                            buyEntries = buyOrderBook;
                            sellEntries = sellOrderBook;

                            if (buyEntries[0].price < sellEntries[0].price) {
                                sellingHappen = false;
                                if (New) {
                                    outputFile << "ORD2300" << orderCount << "," << orderId << "," << instrument << "," << side << "," << price << "," << quantity << "," << "New" << "," << "," << getCurrentTime() << endl;
                                }
                            } else if (buyEntries[0].quantity >= sellEntries[0].quantity) {
                                outputFile << "ORD2300" << orderCount << "," << orderId << "," << instrument << "," << side << "," << buyEntries[0].price << "," << quantity << "," << "Fill" << "," << "," << getCurrentTime() << endl;

                                if (buyEntries[0].quantity == sellEntries[0].quantity) {
                                    outputFile << "ORD2300" << buyEntries[0].receivedorderID << "," << buyEntries[0].orderId << "," << instrument << "," << buyEntries[0].side << "," << buyEntries[0].price << "," << buyEntries[0].quantity << "," << "Fill" << "," << "," << getCurrentTime() << endl;
                                    buyEntries.erase(buyEntries.begin());
                                } else {
                                    outputFile << "ORD2300" << buyEntries[0].receivedorderID << "," << buyEntries[0].orderId << "," << instrument << "," << buyEntries[0].side << "," << buyEntries[0].price << "," << sellEntries[0].quantity << "," << "Fill" << "," << "," << getCurrentTime() << endl;
                                    buyEntries[0].quantity = buyEntries[0].quantity - sellEntries[0].quantity;
                                }
                                sellEntries.erase(sellEntries.begin());
                                sellingHappen = false;
                            } else {
                                outputFile << "ORD2300" << orderCount << "," << orderId << "," << instrument << "," << side << "," << buyEntries[0].price << "," << buyEntries[0].quantity << "," << "Pfill" << "," << "," << getCurrentTime() << endl;
                                outputFile << "ORD2300" << buyEntries[0].receivedorderID << "," << buyEntries[0].orderId << "," << instrument << "," << buyEntries[0].side << "," << buyEntries[0].price << "," << buyEntries[0].quantity << "," << "Fill" << "," << "," << getCurrentTime() << endl;

                                sellEntries[0].quantity = sellEntries[0].quantity - buyEntries[0].quantity;
                                quantity = sellEntries[0].quantity;
                                buyEntries.erase(buyEntries.begin());
                                New = false;
                            }
                            buyOrderBook = buyEntries;
                            sellOrderBook = sellEntries;
                        } else {
                            sellingHappen = false;
                        }
                    }
                }

                orderBooks[instrument] = currentOrderBook;
            }
        }
    }

    inputFile.close();
    outputFile.close();
}

void processOrdersMultiThreaded(const string& inputFilePath, const string& outputFilePath, int chunkSize) {
    // Open the output file to write the headers
    ofstream outputFile(outputFilePath);
    outputFile << "Client Order ID" << "," << "Order ID" << "," << "Instrument" << "," << "Side" << "," << "Price" << "," << "Quantity" << "," << "Status" << "," << "Reason" << "," << "Transaction Time" << "," << endl;
    outputFile.close();  // Close the file after writing headers

    ifstream inputFile(inputFilePath);
    string line;
    int totalOrders = 0;

    while (getline(inputFile, line)) {
        if (totalOrders == 0) {
            totalOrders++;
            continue;
        }
        totalOrders++;
    }

    inputFile.close();

    int numThreads = totalOrders / chunkSize;
    if (totalOrders % chunkSize != 0) {
        numThreads++;
    }

    vector<thread> threads;

    for (int i = 0; i < numThreads; i++) {
        int startOrder = i * chunkSize + 1;
        int endOrder = min((i + 1) * chunkSize, totalOrders);

        threads.emplace_back(processOrderChunk, inputFilePath, outputFilePath, startOrder, endOrder);
    }

    for (auto& thread : threads) {
        thread.join();
    }
}

int main() {
    string inputFilePath = "C:\\Users\\Udith\\OneDrive\\Desktop\\LSEG\\Flower_Exchange_Project\\code\\order1.csv";
    string outputFilePath = "C:\\Users\\Udith\\OneDrive\\Desktop\\LSEG\\Flower_Exchange_Project\\code\\execution_rep1.csv";
    int chunkSize = 1000;

    processOrdersMultiThreaded(inputFilePath, outputFilePath, chunkSize);

    return 0;
}
