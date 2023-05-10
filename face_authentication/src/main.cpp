#include <iostream>
#include <string>
#include <chrono>
#include <NvInfer.h>
#include <NvInferPlugin.h>
#include <opencv2/highgui.hpp>
#include "faceNet.h"
#include "videoStreamer.h"
#include "network.h"
#include "mtcnn.h"
// #include "mqtt_publisher.h"

// Uncomment to print timings in milliseconds
// #define LOG_TIMES


//define thread

// #include <chrono>
// #include <queue>
// #include <set>
// #include <mutex>
// #include <condition_variable>

// class MqttPublisherThread : public std::thread {
// public:
//     MqttPublisherThread(const std::string& brokerAddress, int brokerPort, const std::string& clientId,
//                         const std::string& topic, int qos = 0)
//         : publisher_(brokerAddress, brokerPort, clientId), topic_(topic), qos_(qos), running_(false)
//     {}

//     void start()
//     {
//         running_ = true;
//         std::thread::start();
//     }

//     void stop()
//     {
//         running_ = false;
//         cv_.notify_all();
//         std::thread::join();
//     }

//     void publish(const std::string& message)
//     {
//         std::unique_lock<std::mutex> lock(mutex_);
//         if (set_.insert(message).second) {
//             queue_.push(message);
//             cv_.notify_one();
//         }
//     }

// private:
//     void run()
//     {
//         publisher_.connect();

//         while (running_) {
//             std::unique_lock<std::mutex> lock(mutex_);
//             cv_.wait_for(lock, std::chrono::seconds(1), [this](){ return !queue_.empty() || !running_; });

//             while (!queue_.empty()) {
//                 std::string message = queue_.front();
//                 queue_.pop();
//                 publisher_.publish(topic_, message, qos_);
//                 set_.erase(message);
//             }
//         }

//         publisher_.disconnect();
//     }

//     MqttPublisher publisher_;
//     std::string topic_;
//     int qos_;
//     std::queue<std::string> queue_;
//     std::set<std::string> set_;
//     std::mutex mutex_;
//     std::condition_variable cv_;
//     bool running_;
// };

//End define thread




using namespace nvinfer1;
using namespace nvuffparser;


int main()
{



    //Init Mqttt
    const std::string brokerAddress = "localhost";
    const int brokerPort = 1883;
    const std::string clientId = "face_authentication";
    const std::string topic = "authentication";
    const int qos = 0;

    MqttPublisherThread publisherThread(brokerAddress, brokerPort, clientId, topic, qos);
    publisherThread.start();
   

    Logger gLogger = Logger();
    // Register default TRT plugins (e.g. LRelu_TRT)
    if (!initLibNvInferPlugins(&gLogger, "")) { return 1; }

    // USER DEFINED VALUES
    const string uffFile="../facenetModels/facenet.uff";
    const string engineFile="../facenetModels/facenet.engine";
    DataType dtype = DataType::kHALF;
    //DataType dtype = DataType::kFLOAT;
    bool serializeEngine = true;
    int batchSize = 1;
    int nbFrames = 0;
    int videoFrameWidth = 640;
    int videoFrameHeight = 480;
    int maxFacesPerScene = 5;
    float knownPersonThreshold = 1.;
    bool isCSICam = false;

    // init facenet
    FaceNetClassifier faceNet = FaceNetClassifier(gLogger, dtype, uffFile, engineFile, batchSize, serializeEngine,
            knownPersonThreshold, maxFacesPerScene, videoFrameWidth, videoFrameHeight);

    // init opencv stuff
    VideoStreamer videoStreamer = VideoStreamer(0, videoFrameWidth, videoFrameHeight, 60, isCSICam);
    cv::Mat frame;

    // init mtCNN
    mtcnn mtCNN(videoFrameHeight, videoFrameWidth);

    //init Tlive
    // TLive Live;
    // bool flag = false;
    // Live.LoadModel();

    //init Bbox and allocate memory for "maxFacesPerScene" faces per scene
    std::vector<struct Bbox> outputBbox;
    outputBbox.reserve(maxFacesPerScene);

    // get embeddings of known faces
    std::vector<struct Paths> paths;
    cv::Mat image;
    getFilePaths("../imgs", paths);
    for(int i=0; i < paths.size(); i++) {
        loadInputImage(paths[i].absPath, image, videoFrameWidth, videoFrameHeight);
        outputBbox = mtCNN.findFace(image);
        std::size_t index = paths[i].fileName.find_last_of(".");
        std::string rawName = paths[i].fileName.substr(0,index);
        faceNet.forwardAddFace(image, outputBbox, rawName);
        faceNet.resetVariables();
    }
    outputBbox.clear();

    // loop over frames with inference
    auto globalTimeStart = chrono::steady_clock::now();
    while (true) {
        auto fps_start = chrono::steady_clock::now();
        videoStreamer.getFrame(frame);
        if (frame.empty()) {
            std::cout << "Empty frame! Exiting...\n Try restarting nvargus-daemon by "
                         "doing: sudo systemctl restart nvargus-daemon" << std::endl;
            break;
        }
        auto startMTCNN = chrono::steady_clock::now();
        outputBbox = mtCNN.findFace(frame);
        auto endMTCNN = chrono::steady_clock::now();
        auto startForward = chrono::steady_clock::now();
        faceNet.forward(frame, outputBbox);
        auto endForward = chrono::steady_clock::now();
        // auto startTLive = chrono::steady_clock::now();
        // for(vector<struct Bbox>::iterator it=outputBbox.begin(); it!=outputBbox.end();it++){
        //     if((*it).exist){
        //         float x1 = it->x1;
        //         float y1 = it->y1;
        //         float x2 = it->x2;
        //         float y2 = it->y2;
        //         struct LiveFaceBox LiveBox = {x1, y1, x2, y2};
        //         auto prob = Live.Detect(frame, LiveBox);
        //         if(prob <= 0.9 && flag == false)
        //         {
        //             flag = true;
        //             std::cout<<"Fake face";
        //         }
        //     }
        // }
        // auto endTLive   = chrono::steady_clock::now();
        auto startFeatM = chrono::steady_clock::now();
        faceNet.featureMatching(frame, publisherThread, outputBbox);
        auto endFeatM = chrono::steady_clock::now();
        faceNet.resetVariables();
        
        auto fps_end = chrono::steady_clock::now();
        auto milliseconds = chrono::duration_cast<chrono::milliseconds>(fps_end-fps_start).count();
        float fps = (1000/milliseconds);
        std::string label = cv::format("FPS: %.2f ", fps);
        cv::putText(frame, label, cv::Point(15, 30), cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(0, 0, 0), 2);
        
        cv::imshow("VideoSource", frame);
        nbFrames++;
        outputBbox.clear();
        frame.release();

        char keyboard = cv::waitKey(1);
        if (keyboard == 'q' || keyboard == 27)
            break;
        else if(keyboard == 'n') {
            auto dTimeStart = chrono::steady_clock::now();
            videoStreamer.getFrame(frame);
            outputBbox = mtCNN.findFace(frame);
            cv::imshow("VideoSource", frame);
            faceNet.addNewFace(frame, outputBbox);
            auto dTimeEnd = chrono::steady_clock::now();
            globalTimeStart += (dTimeEnd - dTimeStart);
        }

        #ifdef LOG_TIMES
        std::cout << "mtCNN took " << std::chrono::duration_cast<chrono::milliseconds>(endMTCNN - startMTCNN).count() << "ms\n";
        std::cout << "Forward took " << std::chrono::duration_cast<chrono::milliseconds>(endForward - startForward).count() << "ms\n";
        std::cout << "Feature matching took " << std::chrono::duration_cast<chrono::milliseconds>(endFeatM - startFeatM).count() << "ms\n\n";
        #endif  // LOG_TIMES
    }
    auto globalTimeEnd = chrono::steady_clock::now();
    cv::destroyAllWindows();
    videoStreamer.release();
    auto milliseconds = chrono::duration_cast<chrono::milliseconds>(globalTimeEnd-globalTimeStart).count();
    double seconds = double(milliseconds)/1000.;
    double fps = nbFrames/seconds;

    std::cout << "Counted " << nbFrames << " frames in " << double(milliseconds)/1000. << " seconds!" <<
              " This equals " << fps << "fps.\n";


    // publisherThread.stop();
    return 0;
}

