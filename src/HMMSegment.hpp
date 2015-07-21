#ifndef CPPJIBEA_HMMSEGMENT_H
#define CPPJIBEA_HMMSEGMENT_H

#include <iostream>
#include <fstream>
#include <memory.h>
#include <cassert>
#include "HMMModel.hpp"
#include "SegmentBase.hpp"

namespace CppJieba {
class HMMSegment: public SegmentBase {
 public:
  HMMSegment(const string& filePath) {
    model_ = new HMMModel(filePath);
  }
  HMMSegment(const HMMModel* model) 
  : model_(model), isNeedDestroy_(false) {
  }
  virtual ~HMMSegment() {
    if(isNeedDestroy_) {
      delete model_;
    }
  }

  using SegmentBase::cut;
  bool cut(Unicode::const_iterator begin, Unicode::const_iterator end, vector<Unicode>& res)const {
    Unicode::const_iterator left = begin;
    Unicode::const_iterator right = begin;
    while(right != end) {
      if(*right < 0x80) {
        if(left != right && !cut_(left, right, res)) {
          return false;
        }
        left = right;
        do {
          right = sequentialLetterRule_(left, end);
          if(right != left) {
            break;
          }
          right = numbersRule_(left, end);
          if(right != left) {
            break;
          }
          right ++;
        } while(false);
        res.push_back(Unicode(left, right));
        left = right;
      } else {
        right++;
      }
    }
    if(left != right && !cut_(left, right, res)) {
      return false;
    }
    return true;
  }
  virtual bool cut(Unicode::const_iterator begin, Unicode::const_iterator end, vector<string>& res)const {
    if(begin == end) {
      return false;
    }
    vector<Unicode> words;
    words.reserve(end - begin);
    if(!cut(begin, end, words)) {
      return false;
    }
    size_t offset = res.size();
    res.resize(res.size() + words.size());
    for(size_t i = 0; i < words.size(); i++) {
      TransCode::encode(words[i], res[offset + i]);
    }
    return true;
  }
 private:
  // sequential letters rule
  Unicode::const_iterator sequentialLetterRule_(Unicode::const_iterator begin, Unicode::const_iterator end) const {
    Rune x = *begin;
    if (('a' <= x && x <= 'z') || ('A' <= x && x <= 'Z')) {
      begin ++;
    } else {
      return begin;
    }
    while(begin != end) {
      x = *begin;
      if(('a' <= x && x <= 'z') || ('A' <= x && x <= 'Z') || ('0' <= x && x <= '9')) {
        begin ++;
      } else {
        break;
      }
    }
    return begin;
  }
  //
  Unicode::const_iterator numbersRule_(Unicode::const_iterator begin, Unicode::const_iterator end) const {
    Rune x = *begin;
    if('0' <= x && x <= '9') {
      begin ++;
    } else {
      return begin;
    }
    while(begin != end) {
      x = *begin;
      if( ('0' <= x && x <= '9') || x == '.') {
        begin++;
      } else {
        break;
      }
    }
    return begin;
  }
  bool cut_(Unicode::const_iterator begin, Unicode::const_iterator end, vector<Unicode>& res) const {
    vector<size_t> status;
    if(!viterbi_(begin, end, status)) {
      LogError("viterbi_ failed.");
      return false;
    }

    Unicode::const_iterator left = begin;
    Unicode::const_iterator right;
    for(size_t i = 0; i < status.size(); i++) {
      if(status[i] % 2) { //if(HMMModel::E == status[i] || HMMModel::S == status[i])
        right = begin + i + 1;
        res.push_back(Unicode(left, right));
        left = right;
      }
    }
    return true;
  }

  bool viterbi_(Unicode::const_iterator begin, Unicode::const_iterator end, 
        vector<size_t>& status) const {
    if(begin == end) {
      return false;
    }

    size_t Y = HMMModel::STATUS_SUM;
    size_t X = end - begin;

    size_t XYSize = X * Y;
    size_t now, old, stat;
    double tmp, endE, endS;

    vector<int> path(XYSize);
    vector<double> weight(XYSize);

    //start
    for(size_t y = 0; y < Y; y++) {
      weight[0 + y * X] = model_->startProb[y] + model_->getEmitProb(model_->emitProbVec[y], *begin, MIN_DOUBLE);
      path[0 + y * X] = -1;
    }

    double emitProb;

    for(size_t x = 1; x < X; x++) {
      for(size_t y = 0; y < Y; y++) {
        now = x + y*X;
        weight[now] = MIN_DOUBLE;
        path[now] = HMMModel::E; // warning
        emitProb = model_->getEmitProb(model_->emitProbVec[y], *(begin+x), MIN_DOUBLE);
        for(size_t preY = 0; preY < Y; preY++) {
          old = x - 1 + preY * X;
          tmp = weight[old] + model_->transProb[preY][y] + emitProb;
          if(tmp > weight[now]) {
            weight[now] = tmp;
            path[now] = preY;
          }
        }
      }
    }

    endE = weight[X-1+HMMModel::E*X];
    endS = weight[X-1+HMMModel::S*X];
    stat = 0;
    if(endE >= endS) {
      stat = HMMModel::E;
    } else {
      stat = HMMModel::S;
    }

    status.resize(X);
    for(int x = X -1 ; x >= 0; x--) {
      status[x] = stat;
      stat = path[x + stat*X];
    }

    return true;
  }

 private:
  const HMMModel* model_;
  bool isNeedDestroy_;
}; // class HMMSegment

} // namespace CppJieba

#endif
