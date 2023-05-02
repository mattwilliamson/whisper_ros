
#include <thread>

#include "whisper_ros/whisper.hpp"

using namespace whisper_ros;

Whisper::Whisper(whisper_full_params wparams, std::string model)
    : wparams(wparams) {

  if (whisper_lang_id(wparams.language) == -1) {
    fprintf(stderr, "Unknown language '%s'\n", wparams.language);
    exit(0);
  }

  // init whisper
  this->ctx = whisper_init_from_file(model.c_str());

  if (!whisper_is_multilingual(this->ctx)) {
    if (std::string(wparams.language) != "en" || wparams.translate) {
      wparams.language = "en";
      wparams.translate = false;
      fprintf(stderr, "Model is not multilingual, ignoring language and "
                      "translation options\n");
    }
  }
  fprintf(stderr,
          "Processing, %d threads, lang = %s, task = %s, timestamps = %d ...\n",
          wparams.n_threads, wparams.language,
          wparams.translate ? "translate" : "transcribe",
          wparams.print_timestamps ? 0 : 1);

  fprintf(stderr, "system_info: n_threads = %d / %d | %s\n", wparams.n_threads,
          std::thread::hardware_concurrency(), whisper_print_system_info());
}

Whisper::~Whisper() { whisper_free(this->ctx); }

std::string Whisper::transcribe(const std::vector<float> &pcmf32) {

  float prob = 0.0f;

  if (whisper_full(this->ctx, this->wparams, pcmf32.data(), pcmf32.size())) {
    return "";
  }

  int prob_n = 0;
  std::string result;

  const int n_segments = whisper_full_n_segments(this->ctx);
  for (int i = 0; i < n_segments; ++i) {
    const char *text = whisper_full_get_segment_text(this->ctx, i);

    result += text;

    const int n_tokens = whisper_full_n_tokens(this->ctx, i);
    for (int j = 0; j < n_tokens; ++j) {
      const auto token = whisper_full_get_token_data(this->ctx, i, j);

      prob += token.p;
      ++prob_n;
    }
  }

  if (prob_n > 0) {
    prob /= prob_n;
  }

  return result;
}