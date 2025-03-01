#ifndef NALU_UDP_RECEIVER_H
#define NALU_UDP_RECEIVER_H

#include <vector>
#include <queue>
#include <mutex>
#include <thread>
#include <atomic>
#include <condition_variable>
#include <netinet/in.h>
#include <functional>
#include <chrono>
#include "nalu_udp_data_buffer.h"
#include "nalu_event_collector_params.h"


/**
 * @class NaluUdpReceiver
 * @brief A class for receiving UDP packets, parsing them, and storing the data in a buffer.
 * 
 * This class provides functionality to initialize a UDP socket, receive packets, and store
 * them in a buffer for further processing. It runs in a separate thread to continuously
 * receive data from the network.
 */
class NaluUdpReceiver {
    public:
        /**
         * @brief Constructs a NaluUdpReceiver instance.
         * 
         * @param address The address to bind the receiver to (e.g., "127.0.0.1").
         * @param port The port number to bind the receiver to.
         * @param buffer_size The size of the UDP data buffer to store received data.
         * @param max_packet_size The maximum size of each UDP packet to receive.
         * @param timeout_sec The timeout duration (in seconds) for receiving packets.
         */
        NaluUdpReceiver(const std::string& address, uint16_t port, size_t buffer_size = 1024*1024*100, size_t max_packet_size = 1040, int timeout_sec = 10);
    
        /**
         * @brief Constructs a NaluUdpReceiver instance using parameters encapsulated in a NaluUdpReceiverParams structure.
         * 
         * @param params A structure containing the configuration parameters for the receiver.
         */
        NaluUdpReceiver(const NaluUdpReceiverParams& params);
    
        /**
         * @brief Destroys the NaluUdpReceiver instance and stops the receiver thread.
         * 
         * This method ensures that any resources allocated during the receiver's lifecycle are cleaned up.
         */
        ~NaluUdpReceiver();
    
        /**
         * @brief Starts the UDP receiver in a separate thread.
         * 
         * This method initializes the UDP socket and starts receiving packets in a loop.
         */
        void start();
    
        /**
         * @brief Stops the UDP receiver and terminates the receiver thread.
         * 
         * This method shuts down the receiver and cleans up resources, such as closing the UDP socket.
         */
        void stop();
    
        /**
         * @brief Gets the reference to the NaluUdpDataBuffer where received data is stored.
         * 
         * This buffer contains all received UDP packet data that can be processed further.
         * 
         * @return Reference to the NaluUdpDataBuffer instance.
         */
        NaluUdpDataBuffer& getDataBuffer();
    
    private:
        /**
         * @brief Initializes the UDP socket and binds it to the specified address and port.
         * 
         * This method is called in the receiver loop to set up the socket for receiving data.
         * It handles socket creation, binding, and applying the timeout option.
         * 
         * @throws std::runtime_error if the socket creation or binding fails.
         */
        void initSocket();
    
        /**
         * @brief The main receiving loop that listens for incoming UDP packets.
         * 
         * This method runs in a separate thread and continuously listens for UDP packets.
         * The packets are processed and stored in the `nalu_udp_data_buffer_`.
         * 
         * @throws std::runtime_error if an error occurs during packet reception.
         */
        void receiveLoop();
    
        std::string address_;                /**< The address to bind the receiver to. */
        uint16_t port_;                      /**< The port number to bind the receiver to. */
        int socket_fd_;                      /**< The file descriptor for the UDP socket. */
        std::thread receiver_thread_;        /**< The thread that runs the receiving loop. */
        std::atomic<bool> running_;          /**< Flag indicating whether the receiver is running. */
        NaluUdpDataBuffer nalu_udp_data_buffer_; /**< Data buffer for storing received UDP data. */
        size_t max_packet_size_;             /**< Maximum size for each UDP packet to receive. */
        int timeout_sec_;                    /**< Timeout duration for receiving packets. */
    };
    

#endif // NALU_UDP_RECEIVER_H
