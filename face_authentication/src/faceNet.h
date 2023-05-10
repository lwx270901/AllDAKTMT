#ifndef FACE_RECOGNITION_FACENET_H
#define FACE_RECOGNITION_FACENET_H

#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include <cassert>
#include <numeric>
#include <dirent.h>
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>

#include <NvInfer.h>
#include <NvUffParser.h>
#include <NvInferPlugin.h>
#include "common.h"
#include "pBox.h"

//Publisht thread
#include <chrono>
#include <queue>
#include <set>
#include <mutex>
#include <condition_variable>
#include "mqtt_publisher.h"
#include "TLive.h"

#include <cstdlib>
#include <ctime>


#include "mtcnn.h"

string generateRandomString();


class MqttPublisherThread : public std::thread {
public:
    MqttPublisherThread(const std::string& brokerAddress, int brokerPort, const std::string& clientId,
                        const std::string& topic, int qos = 0)
        : publisher_(brokerAddress, brokerPort, clientId), topic_(topic), qos_(qos), running_(false), message("")
    {}

    void start()
    {
        running_ = true;
        std::thread([this] { this->run(); }).detach();
    }

    void stop()
    {
        running_ = false;
        cv_.notify_all();
        std::thread::join();
    }

    void publish(const std::string& message)
    {
        std::unique_lock<std::mutex> lock(mutex_);
        if(this->message != message)
        {
        	this->message = message;
        	if (set_.insert(message).second) {
		        queue_.push(message);
		        cv_.notify_one();
		    }
        }
        
    }

private:
    void run()
    {
        publisher_.connect();

        while (running_) {
            std::unique_lock<std::mutex> lock(mutex_);
            cv_.wait_for(lock, std::chrono::seconds(10), [this](){ return !queue_.empty() || !running_; });

            while (!queue_.empty()) {
                std::string message = queue_.front();
                queue_.pop();
                publisher_.publish(topic_, message, qos_);
                set_.erase(message);
            }
        }

        publisher_.disconnect();
    }

    MqttPublisher publisher_;
    std::string topic_;
    int qos_;
    std::queue<std::string> queue_;
    std::set<std::string> set_;
    std::mutex mutex_;
    std::condition_variable cv_;
    bool running_;
    string message;
};


//End publish thread

using namespace nvinfer1;
using namespace nvuffparser;

struct CroppedFace {
    cv::Mat faceMat;
    int x1, y1, x2, y2;
    bool isFake = false;
};


class FaceNetClassifier 
{
    public:
        FaceNetClassifier(Logger gLogger, DataType dtype, const string uffFile, const string engineFile, int batchSize,
                bool serializeEngine, float knownPersonThreshold, int maxFacesPerScene, int frameWidth, int frameHeight);
        ~FaceNetClassifier();

        void createOrLoadEngine();
        void getCroppedFacesAndAlign(cv::Mat frame, std::vector<struct Bbox> outputBbox);
        void preprocessFaces();
        void doInference(float* inputData, float* output);
        void forwardAddFace(cv::Mat image, std::vector<struct Bbox> outputBbox, const string className);
        void forward(cv::Mat image, std::vector<struct Bbox> outputBbox);
        void featureMatching(cv::Mat &image, MqttPublisherThread& publisherThread, std::vector<struct Bbox> outputBbox);
        void addNewFace(cv::Mat &image, std::vector<struct Bbox> outputBbox);
        void addNewAbnomal(cv::Mat &image, std::vector<struct Bbox> outputBbox, string event);
        void resetVariables();

    private:
        static int m_classCount;
        int m_INPUT_C;
        int m_INPUT_H;
        int m_INPUT_W;
        int m_frameWidth, m_frameHeight;
        Logger m_gLogger;
        DataType m_dtype;
        string m_uffFile;
        string m_engineFile;
        int m_batchSize;
        bool m_serializeEngine;
        int m_maxFacesPerScene;
        ICudaEngine *m_engine;
        IExecutionContext *m_context;
        float m_output[128];
        std::vector<float> m_embeddings;
        std::vector<struct KnownID> m_knownFaces;
        // std::vector<cv::Mat> m_croppedFaces;
        std::vector<struct CroppedFace> m_croppedFaces;
        float m_knownPersonThresh;
        TLive Live;
        bool fake_flag;
};

float vectors_distance(const std::vector<float>& a, const std::vector<float>& b);
inline unsigned int elementSize(nvinfer1::DataType t);

#endif //FACE_RECOGNITION_FACENET_H
