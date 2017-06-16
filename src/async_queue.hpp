#ifndef ASYNC_QUEUE_HPP
#define ASYNC_QUEUE_HPP

#include <queue>

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/system/error_code.hpp>
#include <boost/thread/mutex.hpp>


enum asyncQueue::errc
{
    stopped,
    ok
};

template<>
struct std::is_error_code_enum<asyncQueue::errc> : public std::true_type {};

template<typename Data>
class AsyncQueue
{
    typedef boost::function<void (const boost::system::error_code& ec, const Data& data)> Callback;

    std::queue<Callback> callbacks_;
    std::queue<Data> theQueue_;

 public:
    AsyncQueue(boost::asio::io_service& ioService) : ioService_(ioService)
        {
        }

 public:
    size_t size() {
        return theQueue_.size();
    }

    void push(const Data& data) {
        {
            boost::mutex::scoped_lock lock(mutex_);
            if (stopped_ || niceStopped_) {
                return;
            } else {
                theQueue_.push(data);
            }
        }
        tryPop();
    }

    void asyncPop(boost::function<void (const boost::system::error_code& ec, const Data& data)> callback)
        {
            {
                boost::mutex::scoped_lock lock(mutex_);
                if (stopped_) {
                    Data d;
                    ioService_.post(boost::bind(AsyncQueue<Data>::stoppedHandler, callback));
                    return;
                }
                callbacks_.push(callback);
            }
            tryPop();
        }

    static void stoppedHandler(boost::function<void (const boost::system::error_code& ec, const Data& data)> callback) {
        Data d;
        boost::system::error_code e;
        callback(e, d);
    }

    void niceClose()
        {
            {
                boost::mutex::scoped_lock lock(mutex_);
                niceStopped_ = true;
            }

            tryPop();
        }

    
    void close() 
        {
            {
                boost::mutex::scoped_lock lock(mutex_);
                if (stopped_) {
                    return;
                }
                stopped_ = true;
            }

            while (!callbacks_.empty()) {
                Callback handler = callbacks_.front();
                callbacks_.pop();
                Data d;
                boost::system::error_code e;
                ioService_.post(boost::bind(handler, e, d));
            }
            clear();
        }

    void clear()
        {
            // swap queues with empty queues
            std::queue<Data>().swap(theQueue_);
        }
    
 private:
    
    void tryPop() {
        Callback handler;
        Data d;
        bool found = false;
        {
            boost::mutex::scoped_lock lock(mutex_);
            if (theQueue_.size() > 0 && callbacks_.size() > 0) {
                handler = callbacks_.front();
                callbacks_.pop();
                d = theQueue_.front();
                theQueue_.pop();
                found = true;
             
            }
        }
        if (found) {
            boost::system::error_code e;
            ioService_.post(boost::bind(handler, e, d));
        } else {
            // This code is used for nice stopping of the queue where all data in the queue is read before it's closed.
            bool emptyHandlers  = false;
            {
                boost::mutex::scoped_lock lock(mutex_);
                if (theQueue_.size() == 0 && niceStopped_ == true) {
                    emptyHandlers = true;
                }
            }
            if (emptyHandlers) {
                while (!callbacks_.empty()) {
                    Callback handler = callbacks_.front();
                    callbacks_.pop();
                    Data d;
                    boost::system::error_code e;
                    ioService_.post(boost::bind(handler, e, d));
                }
            }
        }
    }

    boost::asio::io_service& ioService_;

    boost::mutex mutex_;
    bool stopped_ = false;
    bool niceStopped_ = false;
};

#endif
