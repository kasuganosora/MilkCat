//
// The MIT License (MIT)
//
// Copyright 2013-2014 The MilkCat Project Developers
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
// model_factory.cc --- Created at 2014-04-02
// model_impl.cc --- Created at 2014-09-02
//

#include "common/model_impl.h"

#include "ml/perceptron_model.h"
#include "common/milkcat_config.h"
#include "common/trie_tree.h"
#include "common/static_array.h"
#include "common/static_hashtable.h"
#include "ml/crf_model.h"
#include "ml/hmm_model.h"

namespace milkcat {

// Model filenames
const char *kUnigramIndexFile = "unigram.idx";
const char *kUnigramDataFile = "unigram.bin";
const char *kBigramDataFile = "bigram.bin";
const char *kHmmPosModelFile = "ctb_pos.hmm";
const char *kCrfPosModelFile = "ctb_pos.crf";
const char *kCrfSegModelFile = "ctb_seg.crf";
const char *kOovPropertyFile = "oov_property.idx";
const char *kStopwordFile = "stopword.idx";
const char *kDepengencyFilePrefix = "ctb5_dep";
const char *kDependenctTemplateFile = "depparse.tmpl";

// ---------- Model::Impl ----------

Model::Impl::Impl(const char *model_dir_path):
    model_dir_path_(model_dir_path),
    unigram_index_(NULL),
    user_index_(NULL),
    unigram_cost_(NULL),
    user_cost_(NULL),
    bigram_cost_(NULL),
    seg_model_(NULL),
    crf_pos_model_(NULL),
    hmm_pos_model_(NULL),
    oov_property_(NULL),
    stopword_(NULL),
    dependency_(NULL),
    dependency_feature_(NULL) {
}

Model::Impl::~Impl() {
  delete user_index_;
  user_index_ = NULL;

  delete unigram_index_;
  unigram_index_ = NULL;

  delete unigram_cost_;
  unigram_cost_ = NULL;

  delete user_cost_;
  user_cost_ = NULL;

  delete bigram_cost_;
  bigram_cost_ = NULL;

  delete seg_model_;
  seg_model_ = NULL;

  delete crf_pos_model_;
  crf_pos_model_ = NULL;

  delete hmm_pos_model_;
  hmm_pos_model_ = NULL;

  delete oov_property_;
  oov_property_ = NULL;

  delete stopword_;
  stopword_ = NULL;

  delete dependency_;
  dependency_ = NULL;

  delete dependency_feature_;
  dependency_feature_ = NULL;
}

const TrieTree *Model::Impl::Index(Status *status) {
  mutex.Lock();
  if (unigram_index_ == NULL) {
    std::string model_path = model_dir_path_ + kUnigramIndexFile;
    unigram_index_ = DoubleArrayTrieTree::New(model_path.c_str(), status);
  }
  mutex.Unlock();
  return unigram_index_;
}

void Model::Impl::LoadUserDictionary(const char *path, Status *status) {
  char line[1024], word[1024];
  std::string errmsg;
  ReadableFile *fd;
  float default_cost = kDefaultCost, cost;
  std::vector<float> user_costs;
  std::map<std::string, int> term_ids;

  if (status->ok()) fd = ReadableFile::New(path, status);
  while (status->ok() && !fd->Eof()) {
    fd->ReadLine(line, sizeof(line), status);
    if (status->ok()) {
      char *p = strchr(line, ' ');

      // Checks if the entry has a cost
      if (p != NULL) {
        strlcpy(word, line, p - line + 1);
        trim(word);
        trim(p);
        cost = static_cast<float>(atof(p));
      } else {
        strlcpy(word, line, sizeof(word));
        trim(word);
        cost = default_cost;
      }
      term_ids.insert(std::pair<std::string, int>(
          word, 
          kUserTermIdStart + term_ids.size()));
      user_costs.push_back(cost);
    }
  }

  if (status->ok() && term_ids.size() == 0) {
    errmsg = std::string("User dictionary ") + path + " is empty.";
    *status = Status::Corruption(errmsg.c_str());
  }


  // Build the index and the cost array from user dictionary
  if (status->ok()) {
    delete user_index_;
    user_index_ = DoubleArrayTrieTree::NewFromMap(term_ids);
  }
  if (status->ok()) {
    delete user_cost_;
    user_cost_ = StaticArray<float>::NewFromArray(user_costs.data(),
                                                  user_costs.size());
  }

  delete fd;
}

bool Model::Impl::SetUserDictionary(const char *path) {
  Status status;
  mutex.Lock();
  LoadUserDictionary(path, &status);
  mutex.Unlock();

  if (status.ok()) {
    return true;
  } else {
    return false;
  }
}

void Model::Impl::SetUserDictionary(
    const unordered_map<std::string, float> &words) {
  std::map<std::string, int> term_ids;
  std::vector<float> costs;

  mutex.Lock();
  for (unordered_map<std::string, float>::const_iterator
       it = words.begin(); it != words.end(); ++it) {
    term_ids.insert(std::make_pair(
        it->first, 
        kUserTermIdStart + term_ids.size()));
    costs.push_back(it->second);
  }

  delete user_index_;
  user_index_ = DoubleArrayTrieTree::NewFromMap(term_ids);

  delete user_cost_;
  user_cost_ = StaticArray<float>::NewFromArray(costs.data(), costs.size());
  mutex.Unlock();  
}

const TrieTree *Model::Impl::UserIndex(Status *status) {
  if (user_index_) {
    return user_index_;
  } else {
    *status = Status::RuntimeError("No user dictionary");
    return NULL;
  }
}

const StaticArray<float> *Model::Impl::UserCost(Status *status) {
  if (user_cost_) {
    return user_cost_;
  } else {
    *status = Status::RuntimeError("No user dictionary");
    return NULL;
  }
}

const StaticArray<float> *Model::Impl::UnigramCost(Status *status) {
  mutex.Lock();
  if (unigram_cost_ == NULL) {
    std::string model_path = model_dir_path_ + kUnigramDataFile;
    unigram_cost_ = StaticArray<float>::New(model_path.c_str(), status);
  }
  mutex.Unlock();
  return unigram_cost_;
}

const StaticHashTable<int64_t, float> *Model::Impl::BigramCost(
    Status *status) {
  mutex.Lock();
  if (bigram_cost_ == NULL) {
    std::string model_path = model_dir_path_ + kBigramDataFile;
    bigram_cost_ = StaticHashTable<int64_t, float>::New(model_path.c_str(),
                                                        status);
  }
  mutex.Unlock();
  return bigram_cost_;
}

const CRFModel *Model::Impl::CRFSegModel(Status *status) {
  mutex.Lock();
  if (seg_model_ == NULL) {
    std::string model_path = model_dir_path_ + kCrfSegModelFile;
    seg_model_ = CRFModel::New(model_path.c_str(), status);
  }
  mutex.Unlock();
  return seg_model_;
}

const CRFModel *Model::Impl::CRFPosModel(Status *status) {
  mutex.Lock();
  if (crf_pos_model_ == NULL) {
    std::string model_path = model_dir_path_ + kCrfPosModelFile;
    crf_pos_model_ = CRFModel::New(model_path.c_str(), status);
  }
  mutex.Unlock();
  return crf_pos_model_;
}

const HMMModel *Model::Impl::HMMPosModel(Status *status) {
  mutex.Lock();
  if (hmm_pos_model_ == NULL) {
    std::string model_path = model_dir_path_ + kHmmPosModelFile;
    hmm_pos_model_ = HMMModel::New(model_path.c_str(), status);
  }
  mutex.Unlock();
  return hmm_pos_model_;
}

const TrieTree *Model::Impl::OOVProperty(Status *status) {
  mutex.Lock();
  if (oov_property_ == NULL) {
    std::string model_path = model_dir_path_ + kOovPropertyFile;
    oov_property_ = DoubleArrayTrieTree::New(model_path.c_str(), status);
  }
  mutex.Unlock();
  return oov_property_;
}

const TrieTree *Model::Impl::Stopword(Status *status) {
  mutex.Lock();
  if (stopword_ == NULL) {
    std::string model_path = model_dir_path_ + kStopwordFile;
    stopword_ = DoubleArrayTrieTree::New(model_path.c_str(), status);
  }
  mutex.Unlock();
  return stopword_;
}

PerceptronModel *Model::Impl::DependencyModel(Status *status) {
  mutex.Lock();
  if (dependency_ == NULL) {
    std::string prefix = model_dir_path_ + kDepengencyFilePrefix;
    dependency_ = PerceptronModel::Open(prefix.c_str(), status);
  }
  mutex.Unlock();
  return dependency_;  
}

DependencyParser::FeatureTemplate *
Model::Impl::DependencyTemplate(Status *status) {
  mutex.Lock();
  if (dependency_feature_ == NULL) {
    std::string prefix = model_dir_path_ + kDependenctTemplateFile;
    dependency_feature_ = DependencyParser::FeatureTemplate::Open(
        prefix.c_str(),
        status);
  }
  mutex.Unlock();
  return dependency_feature_;  
}

}  // namespace milkcat