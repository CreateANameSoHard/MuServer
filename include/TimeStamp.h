#pragma once
#include<iostream>
#include<string>
#include<sys/time.h>
#include<stdint.h>


class TimeStamp {
public:
  TimeStamp();  // default constructor
  explicit TimeStamp(int64_t microSecondsSinceEpoch);   // constructor with microseconds
  static TimeStamp now();  // get current time
  std::string toString() const;  // convert to string
private:
  int64_t microSecondsSinceEpoch_;  // microseconds since epoch
};