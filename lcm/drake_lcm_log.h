#pragma once

#include <atomic>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

#include "lcm/lcm-cpp.hpp"

#include "drake/common/drake_copyable.h"
#include "drake/lcm/drake_lcm_interface.h"

namespace drake {
namespace lcm {

/**
 * A LCM interface for logging LCM messages to a file or playing back from a
 * existing log. Note the user is responsible for offsetting the clock used
 * to generate the log and the clock used for playback. For example, if the log
 * is generated by some external logger (the lcm-logger binary), which uses the
 * unix epoch time clock to record message arrival time, the user needs to
 * offset those timestamps properly to match and the clock used for playback.
 */
class DrakeLcmLog : public DrakeLcmInterface {
 public:
  DRAKE_NO_COPY_NO_MOVE_NO_ASSIGN(DrakeLcmLog);

  /**
   * Constructs a DrakeLcmLog.
   * @param file_name Log's file name for reading or writing.
   * @param is_write If false, this instance reads from the Lcm log
   * identified by @p file_name. If true, this instance writes to the Lcm log
   * whose name is given by @p file_name.
   * @param overwrite_publish_time_with_system_clock This parameter only
   * affects the Publish method in write-only mode. If true, override the
   * `second` parameter passed to Publish method, and use host system's clock
   * to generate the timestamp for the logged message. This is used to mimic
   * lcm-logger's behavior. It also implicitly records how fast the messages
   * are generated in real time.
   *
   * @throws std::exception if unable to open file.
   */
  DrakeLcmLog(const std::string& file_name, bool is_write,
              bool overwrite_publish_time_with_system_clock = false);

  /**
   * Writes an entry occurred at @p timestamp with content @p data to the log
   * file. The current implementation blocks until writing is done.
   * @param channel Channel name.
   * @param data Pointer to raw bytes.
   * @param data_size Number of bytes in @p data.
   * @param time_sec Time in seconds when the message is published. Since
   * messages are save to the log file in the order of Publish calls, this
   * function should only be called with non-decreasing @p second. Note that
   * this parameter can be overwritten by the host system's clock if
   * `overwrite_publish_time_with_system_clock` is true at construction time.
   *
   * @throws std::exception if this instance is not constructed in write-only
   * mode.
   */
  void Publish(const std::string& channel, const void* data, int data_size,
               std::optional<double> time_sec) override;

  /**
   * Subscribes @p handler to @p channel. Multiple handlers can subscribe to the
   * same channel.
   *
   * @throws std::exception if this instance is not constructed in read-only
   * mode.
   *
   * @return nullptr because this implementation does not support unsubscribe.
   */
  std::shared_ptr<DrakeSubscriptionInterface> Subscribe(
      const std::string& channel, HandlerFunction handler) override;

  /**
   * This is a no-op for Read mode, and an exception in Write mode.
   */
  int HandleSubscriptions(int) override;

  /**
   * Returns the time in seconds for the next logged message's occurrence time
   * or infinity if there are no more messages in the current log.
   *
   * @throws std::exception if this instance is not constructed in read-only
   * mode.
   */
  double GetNextMessageTime() const;

  /**
   * Let `MSG` be the next message event in the log, if @p current_time matches
   * `MSG`'s timestamp, for every DrakeLcmMessageHandlerInterface `sub` that's
   * subscribed to `MSG`'s channel, invoke `sub`'s HandleMessage method. Then,
   * this function advances the log by exactly one message. This function does
   * nothing if `MSG` is null (end of log) or @p current_time does not match
   * `MSG`'s timestamp.
   *
   * @throws std::exception if this instance is not constructed in read-only
   * mode.
   */
  void DispatchMessageAndAdvanceLog(double current_time);

  /**
   * Returns true if this instance is constructed in write-only mode.
   */
  bool is_write() const { return is_write_; }

  /**
   * Converts @p timestamp (in microseconds) to time (in seconds) relative to
   * the starting time passed to the constructor.
   */
  double timestamp_to_second(uint64_t timestamp) const {
    return static_cast<double>(timestamp) / 1e6;
  }

  /**
   * Converts time (in seconds) relative to the starting time passed to the
   * constructor to a timestamp in microseconds.
   */
  uint64_t second_to_timestamp(double sec) const {
    return static_cast<uint64_t>(sec * 1e6);
  }

 private:
  void OnHandleSubscriptionsError(const std::string&) override;

  const bool is_write_;
  const bool overwrite_publish_time_with_system_clock_;

  // TODO(jwnimmer-tri) It is not clear to me why this class needs a mutex
  // (i.e., where multiple threads are coming from).  That factor needs to be
  // re-discovered and then documented somewhere.

  // This mutes guards access to all of the below member fields.
  mutable std::mutex mutex_;
  std::multimap<std::string, DrakeLcmInterface::HandlerFunction> subscriptions_;
  std::unique_ptr<::lcm::LogFile> log_;
  const ::lcm::LogEvent* next_event_{nullptr};
};

}  // namespace lcm
}  // namespace drake
