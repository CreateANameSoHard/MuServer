#pragma once
#include <memory>
#include <typeinfo>
#include <cassert>

class Any {
public:
    Any() : ptr_(nullptr) {}
    
    template<typename T>
    Any(const T& value) : ptr_(new Holder<T>(value)) {}
    
    Any(const Any& other) : ptr_(other.ptr_ ? other.ptr_->clone() : nullptr) {} //拷贝
    
    ~Any() = default;
    //copy-and-swap
    Any& operator=(const Any& other) {
        if (this != &other) {
            Any(other).swap(*this);
        }
        return *this;
    }
    
    void swap(Any& other) {
        std::swap(ptr_, other.ptr_);
    }
    
    bool has_value() const { return ptr_ != nullptr; }
    
    template<typename T>
    T& as() {
        if (!has_value()) {
            throw std::bad_cast();
        }
        //获取holder的源指针
        auto* holder = dynamic_cast<Holder<T>*>(ptr_.get());
        if (!holder) {
            throw std::bad_cast();
        }
        return holder->value_;
    }
    
    template<typename T>
    const T& as() const {
        return const_cast<Any*>(this)->as<T>();
    }
    
private:
    struct Placeholder {
        virtual ~Placeholder() = default;
        virtual std::unique_ptr<Placeholder> clone() const = 0;
    };
    
    template<typename T>
    struct Holder : public Placeholder {
        Holder(const T& val) : value_(val) {}
        std::unique_ptr<Placeholder> clone() const override {
            return std::unique_ptr<Placeholder>(new Holder<T>(value_));
        }
        T value_;
    };
    
    std::unique_ptr<Placeholder> ptr_;
};