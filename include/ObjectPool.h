#include <memory>
#include <deque>
#include <functional>
#include <mutex>

template <class T>
class ObjectPool
{
public:
    using CreateFunc = std::function<T *()>;
    using ResetFunc = std::function<void(T *)>;

    // 池里的对象默认是用无参的构造函数
    explicit ObjectPool(size_t maxSize, CreateFunc createFunc = []
                                        { return new T(); },
                        ResetFunc resetFunc = [](T *) {})
        : maxSize_(maxSize), pool_(), mutex_(), createFunc_(std::move(createFunc)), resetFunc_(std::move(resetFunc))
    {
    }

    ~ObjectPool()
    {
        while(!pool_.empty())
        {
            delete pool_.front(); //deque存的是指针
            pool_.pop_front();
        }
    }

    std::unique_ptr<T, std::function<void(T*)>> get()
    {
        T* obj = nullptr;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if(!pool_.empty())
            {
                obj = pool_.front();
                pool_.pop_front();
            }
        }

        if(!obj)
            obj = createFunc_();
        //通过删除器删除
        return std::unique_ptr<T, std::function<void(T*)>>(
            obj,
            [this](T* ptr)
            {
                release(ptr);
            }
        )
    }

private:
    void release(T* obj)
    {
        resetFunc_(obj);
        if(pool_.size() < maxSize_)
        {
            pool_.emplace_back(obj);
        }
        else
        {
            delete obj;
        }
    }

    size_t maxSize_;
    std::deque<T*> pool_;

    std::mutex mutex_;
    CreateFunc createFunc_;
    ResetFunc resetFunc_;
};