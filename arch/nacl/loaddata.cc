#include <stdio.h>
#include "MainThreadRunner.h"
#include <ppapi/c/pp_errors.h>
#include <ppapi/cpp/completion_callback.h>
#include <ppapi/cpp/url_loader.h>
#include <ppapi/cpp/url_request_info.h>
#include <ppapi/cpp/url_response_info.h>
#if ADHOC_PPAPI_VERSION >= 18
#include <ppapi/utility/completion_callback_factory.h>
#endif
#include <string>
#include <vector>
#include "loaddata.h"

class UrlLoaderJob : public MainThreadJob {
 public:
  UrlLoaderJob();
  ~UrlLoaderJob();

  // AppendField() adds the key and value to the request body
  // The key is the name of the form-data for Content-Disposition, and the
  // value is the associated data in the header. AppendField(key, val) adds
  // "Content-Disposition: form-data; name="key"" to the request body.
  // value is then inserted into the request body following the key.
  void AppendField(const std::string& key, const std::string& value);

  // AppendDataField() functions similarly to AppendField().  AppendDatafield()
  // adds the key and data to the request body in the same way.  length is the
  // number of bytes of data.  If free is set to true, data will be deleted
  // in the destructor.
  void AppendDataField(const std::string& key, const void *data, size_t length,
                       bool free);

  // Run() executes the URL request.  This method is called by a
  // MainThreadRunner object.
  void Run(MainThreadRunner::JobEntry *e);

  // Result is used to store information about the response to a URL request.
  // length stores how many bytes are in the content of the response.
  // status_code stores the HTTP status code of the response, e.g. 200
  struct Result {
    int length;
    int status_code;
  };

  void set_content_range(int start, int length) {
    start_ = start;
    length_ = length;
  }
  void set_method(const std::string& method) { method_ = method; }
  void set_result_dst(Result *result_dst) { result_dst_ = result_dst; }
  void set_url(const std::string& url) { url_ = url; }

  char *get_dst() { return dst_; }
  int get_dst_length() { return dstpos_; }

 private:
  struct FieldEntry {
    std:: string key;
    const char *data;
    int length;
    bool free_me;
  };

  void InitRequest(pp::URLRequestInfo *request);
  void OnOpen(int32_t result);
  void OnRead(int32_t result);
  void ReadMore();
  void ProcessResponseInfo(const pp::URLResponseInfo& response_info);
  void ProcessBytes(const char* bytes, int32_t length);
  int ParseContentLength(const std::string& headers);

  pp::CompletionCallbackFactory<UrlLoaderJob> *factory_;
  pp::URLLoader *loader_;
  MainThreadRunner::JobEntry *job_entry_;
  std::vector<FieldEntry> fields_;
  std::string url_;
  std::string method_;
  bool did_open_;
  int start_;
  int length_;
  Result *result_dst_;
  char buf_[4096];
  char *dst_;
  int dstsize_;
  int dstpos_;
};

/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#include <stdio.h>
#include <string.h>

#define BOUNDARY_STRING_HEADER "n4(1+60061E(|20|^^|34TW1234567890!$\n"
#define BOUNDARY_STRING_SEP "--n4(1+60061E(|20|^^|34TW1234567890!$\r\n"
#define BOUNDARY_STRING_END "--n4(1+60061E(|20|^^|34TW1234567890!$--\r\n"

UrlLoaderJob::UrlLoaderJob() {
  start_ = -1;
  did_open_ = false;
  result_dst_ = NULL;
  dst_ = NULL;
  method_ = "GET";
  dstsize_ = 0;
  dstpos_ = 0;
}

UrlLoaderJob::~UrlLoaderJob() {
  for (std::vector<FieldEntry>::iterator it = fields_.begin();
      it != fields_.end(); ++it) {
    if (it->free_me) {
      delete[] it->data;
    }
  }
}

void UrlLoaderJob::AppendField(const std::string& key,
                               const std::string& value) {
  char *data = new char[value.size()];
  memcpy(data, value.c_str(), value.size());
  AppendDataField(key, data, value.size(), true);
}

void UrlLoaderJob::AppendDataField(const std::string& key, const void *data,
                                   size_t length, bool free) {
  FieldEntry e;
  e.key = key;
  e.data = reinterpret_cast<const char *>(data);
  e.length = length;
  e.free_me = free;
  fields_.push_back(e);
}

void UrlLoaderJob::Run(MainThreadRunner::JobEntry *e) {
  job_entry_ = e;
  loader_ = new pp::URLLoader(MainThreadRunner::ExtractPepperInstance(e));
  factory_ = new pp::CompletionCallbackFactory<UrlLoaderJob>(this);
  pp::CompletionCallback cc = factory_->NewCallback(&UrlLoaderJob::OnOpen);
  pp::URLRequestInfo request(MainThreadRunner::ExtractPepperInstance(e));
  InitRequest(&request);
  int32_t rv = loader_->Open(request, cc);
  if (rv != PP_OK_COMPLETIONPENDING) {
    cc.Run(rv);
  }
}

void UrlLoaderJob::InitRequest(pp::URLRequestInfo *request) {
  request->SetURL(url_);
  request->SetMethod(method_);
  request->SetFollowRedirects(true);
  request->SetAllowCredentials(true);
  request->SetAllowCrossOriginRequests(true);
  std::string headers;
  if (start_ >= 0) {
    headers += "Content-Range: ";
    headers += start_;
    headers += "-";
    headers += (start_ + length_);
    headers += "/*\n";
  }
  if (method_ == "POST") {
    headers += "Content-Type: multipart/form-data; boundary=";
    headers += BOUNDARY_STRING_HEADER;
    std::vector<FieldEntry>::iterator it;
    for (it = fields_.begin(); it != fields_.end(); ++it) {
      if (it != fields_.begin()) {
        request->AppendDataToBody("\n", 1);
      }
      request->AppendDataToBody(BOUNDARY_STRING_SEP,
                                sizeof(BOUNDARY_STRING_SEP) - 1);

      std::string line = "Content-Disposition: form-data; name=\"" + it->key
                         + "\"; filename=\"" + it->key
                         + "\"\nContent-Type: application/octet-stream\n\n";

      request->AppendDataToBody(line.c_str(), line.size());
      request->AppendDataToBody(it->data, it->length);
    }
    request->AppendDataToBody("\n", 1);
    request->AppendDataToBody(BOUNDARY_STRING_END,
                              sizeof(BOUNDARY_STRING_END) - 1);
  }
  request->SetHeaders(headers);
}

void UrlLoaderJob::OnOpen(int32_t result) {
  if (result >= 0) {
    ReadMore();
  } else {
    // TODO(arbenson): provide a more meaningful completion result
    MainThreadRunner::ResultCompletion(job_entry_, 0);
  }
}

void UrlLoaderJob::OnRead(int32_t result) {
  if (result > 0) {
    ProcessBytes(buf_, result);
    ReadMore();
  } else if (result == PP_OK && !did_open_) {
    // Headers are available, and we can start reading the body.
    did_open_ = true;
    ProcessResponseInfo(loader_->GetResponseInfo());
    ReadMore();
  } else {
    // TODO(arbenson): provide a more meaningful completion result
    MainThreadRunner::ResultCompletion(job_entry_, 0);
  }
}

void UrlLoaderJob::ReadMore() {
  pp::CompletionCallback cc = factory_->NewCallback(&UrlLoaderJob::OnRead);
  int32_t rv = loader_->ReadResponseBody(buf_, sizeof(buf_), cc);
  if (rv != PP_OK_COMPLETIONPENDING) {
    cc.Run(rv);
  }
}

int UrlLoaderJob::ParseContentLength(const std::string& headers) {
  static const char *kCONTENT_LENGTH = "Content-Length: ";
  size_t begin = headers.find(kCONTENT_LENGTH);
  size_t end = headers.find("\n", begin);
  if (begin == std::string::npos || end == std::string::npos) {
    return -1;
  }
  std::string line = headers.substr(begin, end);
  int cl;
  if (!sscanf(line.c_str(), "Content-Length: %d\n", &cl)) {
    return -1;
  }
  return cl;
}

void UrlLoaderJob::ProcessResponseInfo(
    const pp::URLResponseInfo& response_info) {
  if (!result_dst_) {
    return;
  }
  result_dst_->status_code = response_info.GetStatusCode();
  std::string headers = response_info.GetHeaders().AsString();
  result_dst_->length = ParseContentLength(headers);
}

void UrlLoaderJob::ProcessBytes(const char* bytes, int32_t length) {
  assert(length >= 0);
  if (dstpos_ + length > dstsize_) {
    dstsize_ = dstsize_ + (dstsize_ >> 1) + length;
    dst_ = (char *)realloc(dst_, dstsize_);
  }
  memcpy(dst_ + dstpos_, bytes, length);
  dstpos_ += length;
}

const char *loadurl(void *runner, const char *url, int *size) {
  MainThreadRunner *r = reinterpret_cast<MainThreadRunner *>(runner);
  UrlLoaderJob *job = new UrlLoaderJob;
  job->set_url(url);
  r->RunJob(job);
  *size = job->get_dst_length();
  return job->get_dst();
}
