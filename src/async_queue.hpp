#ifndef ASYNC_QUEUE_HPP
#define ASYNC_QUEUE_HPP

#include <queue>

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/system/error_code.hpp>
#include <boost/thread/mutex.hpp>


// =============== ERROR CODE DEFINITIONS ===============
enum class queue_error_code : int {
    stopped = 1,
    ok
};

class queue_error_category_impl : public std::error_category
{
    const char* name () const noexcept override {
        return "AsyncQueue";
    }

    std::string  message (int ec) const override {
        std::string msg;
        switch (queue_error_code(ec)) {
        case queue_error_code::stopped:
            msg = "Queue stopped.";
            break;
        case queue_error_code::ok:
            msg = "OK";
            break;
        default:
            msg = "unknown.";
        }
        return msg;
    }

    std::error_condition default_error_condition (int ec) const noexcept override
    {
        return std::error_condition(ec, *this);
    }
};

// unique instance of the error category
struct queue_error_category
{
    static const std::error_category& instance () noexcept
    {
        static queue_error_category_impl category;
        return category;
    }
};

// overload for error code creation
inline std::error_code make_error_code (queue_error_code ec) noexcept
{
    return std::error_code(static_cast<int>(ec), queue_error_category::instance());
}

// overload for error condition creation
inline std::error_condition make_error_condition (queue_error_code ec) noexcept
{
    return std::error_condition(static_cast<int>(ec), queue_error_category::instance());
}

/**
 * Exception type thrown by the lib.
 */
class queue_error : public virtual std::runtime_error
{
public:
    explicit queue_error (queue_error_code ec) noexcept : std::runtime_error("AsyncQueue "), internal_code(make_error_code(ec)) {}

    const char* what () const noexcept override {
        return internal_code.message().c_str();
    }

    std::error_code code () const noexcept {
        return internal_code;
    }

private:
    std::error_code internal_code;
};

// specialization for error code enumerations
// must be done in the std namespace

    namespace std
    {
    template <>
    struct is_error_code_enum<queue_error_code> : public true_type { };
    }

// =============== ASYNC QUEUE DEFINITION ===============


template<typename Data>
class AsyncQueue
{
    typedef boost::function<void (const std::error_code& ec, const Data& data)> Callback;

    Callback callback_;
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

    void asyncPop(boost::function<void (const std::error_code& ec, const Data& data)> callback)
        {
            {
                boost::mutex::scoped_lock lock(mutex_);
                if (stopped_) {
                    Data d;
                    ioService_.post(boost::bind(AsyncQueue<Data>::stoppedHandler, callback));
                    return;
                }
                callback_ = callback;
            }
            tryPop();
        }

    static void stoppedHandler(boost::function<void (const std::error_code& ec, const Data& data)> callback) {
        Data d;
        std::error_code e = queue_error_code::stopped;
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
            if(callback_) {
                Data d;
                std::error_code e = queue_error_code::stopped;
                ioService_.post(boost::bind(callback_, e, d));
                callback_ = NULL;
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
        Data d;
        bool found = false;
        {
            boost::mutex::scoped_lock lock(mutex_);
            if (theQueue_.size() > 0 && callback_) {
                d = theQueue_.front();
                theQueue_.pop();
                found = true;
            }
        }
        if (found) {
            std::error_code e = queue_error_code::ok;
            ioService_.post(boost::bind(callback_, e, d));
            callback_ = NULL;
        } else {
            // This code is used for nice stopping of the queue where all data in the queue is read before it's closed.
            boost::mutex::scoped_lock lock(mutex_);
            if (theQueue_.size() == 0 && niceStopped_ == true && callback_) {
                Data d;
                std::error_code e = queue_error_code::stopped;
                ioService_.post(boost::bind(callback_, e, d));
                callback_ = NULL;
            }
        }
    }

    boost::asio::io_service& ioService_;

    boost::mutex mutex_;
    bool stopped_ = false;
    bool niceStopped_ = false;
};

#endif
