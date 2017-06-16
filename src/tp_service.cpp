#include "tp_service.hpp"
#include <mutex>


TpService::TpService(){
    
}

std::shared_ptr<TpService> TpService::getInstance(){
    static std::shared_ptr<TpService> instance_;
    static std::mutex mutex_;
    std::lock_guard<std::mutex> lock(mutex_);
    if ( !instance_ ) {
        instance_ = std::make_shared<TpService>();
        instance_->init(4);
    }
    return instance_;
}

TpService::~TpService(){
    ioService_.stop();
}

void TpService::init(int size){
    boost::asio::io_service::work work(ioService_);
    work_.reset(new boost::asio::io_service::work(ioService_));
    for (int i = 0; i<size; i++){
        threadpool_.create_thread(
            boost::bind(&boost::asio::io_service::run, &ioService_)
            );
    }
}
