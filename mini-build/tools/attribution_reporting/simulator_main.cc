// Copyright 2022 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <iostream>
#include <memory>
#include <string>

#include "base/command_line.h"
#include "base/containers/contains.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/strings/abseil_string_number_conversions.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_piece.h"
#include "base/values.h"
#include "components/version_info/version_info.h"
#include "content/public/browser/attribution_reporting.h"
#include "content/public/common/network_service_util.h"
#include "content/public/test/attribution_simulator.h"
#include "content/public/test/content_test_suite_base.h"
#include "content/public/test/test_content_client_initializer.h"
#include "mojo/core/embedder/embedder.h"
#include "third_party/abseil-cpp/absl/numeric/int128.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace {

constexpr char kSwitchHelp[] = "help";
constexpr char kSwitchHelpShort[] = "h";

constexpr char kSwitchVersion[] = "version";
constexpr char kSwitchVersionShort[] = "v";

constexpr char kSwitchDelayMode[] = "delay_mode";
constexpr char kSwitchNoiseMode[] = "noise_mode";
constexpr char kSwitchNoiseSeed[] = "noise_seed";
constexpr char kSwitchRemoveReportIds[] = "remove_report_ids";
constexpr char kSwitchInputMode[] = "input_mode";
constexpr char kSwitchCopyInputToOutput[] = "copy_input_to_output";
constexpr char kSwitchReportTimeFormat[] = "report_time_format";
constexpr char kSwitchRandomizedResponseRateNavigation[] =
    "randomized_response_rate_navigation";
constexpr char kSwitchRandomizedResponseRateEvent[] =
    "randomized_response_rate_event";
constexpr char kSwitchRemoveAssembledReport[] = "remove_assembled_report";

constexpr const char* kAllowedSwitches[] = {
    kSwitchHelp,
    kSwitchHelpShort,
    kSwitchVersion,
    kSwitchVersionShort,

    kSwitchDelayMode,
    kSwitchNoiseMode,
    kSwitchNoiseSeed,
    kSwitchRemoveReportIds,
    kSwitchInputMode,
    kSwitchCopyInputToOutput,
    kSwitchReportTimeFormat,
    kSwitchRandomizedResponseRateNavigation,
    kSwitchRandomizedResponseRateEvent,
};

constexpr char kHelpMsg[] = R"(
attribution_reporting_simulator
  [--copy_input_to_output]
  [--delay_mode=<mode>]
  [--noise_mode=<mode>]
  [--noise_seed=<seed>]
  [--randomized_response_rate_event=<rate>]
  [--randomized_response_rate_navigation=<rate>]
  [--input_mode=<input_mode>]
  [--remove_report_ids]
  [--report_time_format=<format>]
  [--remove_assembled_report]

attribution_reporting_simulator is a command-line tool that simulates the
Attribution Reporting API for for sources and triggers specified in an input
file. It writes the generated reports, if any, to stdout, with associated
metadata.

Sources and triggers are registered in chronological order according to their
`source_time` and `trigger_time` fields, respectively.

Input is received by the utility from stdin. The input must be valid JSON
containing sources and triggers to register in the simulation. The format
is described below in detail.

Learn more about the Attribution Reporting API at
https://github.com/WICG/conversion-measurement-api#attribution-reporting-api.

Learn about the meaning of the input and output fields at
https://github.com/WICG/conversion-measurement-api/blob/main/EVENT.md.

Switches:
  --copy_input_to_output    - Optional. If present, the input is copied to the
                              output in a top-level field called `input`.

  --delay_mode=<mode>       - Optional. One of `default` or `none`. Defaults to
                              `default`.

                              default: Reports are sent in reporting windows
                              some time after attribution is triggered.

                              none: Reports are sent immediately after
                              attribution is triggered.

  --noise_mode=<mode>       - Optional. One of `default` or `none`. Defaults to
                              `default`.

                              default: Sources are subject to randomized
                              response, reports within a reporting window are
                              shuffled.

                              none: None of the above applies.

  --noise_seed=<seed>       - Optional 128-bit hex string. If set, the value is
                              used to seed the random number generator used for
                              noise; in this case, the algorithm is
                              XorShift128+. If not set, the default source of
                              randomness is used for noising and the
                              simulation's output may vary between runs.

                              May only be set if `noise_mode` is `noise`.

  --input_mode=<input_mode> - Optional. Either `single` (default) or `multi`.
                              single: the input file must conform to the JSON
                              input format below. Output will conform to the
                              JSON output below.
                              multi: Each line in the input file must
                              conform to the input format below. Each output
                              line will conform to the JSON output format.
                              Input lines are processed independently,
                              simulating multiple users.
                              See https://jsonlines.org/.

  --randomized_response_rate_event=<rate>
                            - Optional double in the range [0, 1]. If present,
                              overrides the default randomized response rate
                              for event sources.

  --randomized_response_rate_navigation=<rate>
                            - Optional double in the range [0, 1]. If present,
                              overrides the default randomized response rate
                              for navigation sources.

  --remove_report_ids       - Optional. If present, removes the `report_id`
                              field from report bodies, as they are randomly
                              generated. Use this switch to make the tool's
                              output more deterministic.

  --report_time_format=<format>
                            - Optional. Either `seconds_since_unix_epoch`
                              (default) or `iso8601`. Controls the report time
                              output format.

                              `seconds_since_unix_epoch`: Report times are
                              integer seconds since the Unix epoch, e.g.
                              1643408373.

                              `iso8601`: Report times are ISO 8601 strings,
                              e.g. "2022-01-28T22:19:33.000Z".

  --remove_assembled_report - Optional. If present, removes the `shared_info`,
                              `aggregation_service_payloads` and
                              `source_registration_time` fields from
                              aggregatable report bodies, as they are randomly
                              generated. Use this switch to make the tool's
                              output more deterministic.

  --version                 - Outputs the tool version and exits.

Input JSON format:

{
  // List of zero or more sources to register.
  "sources": [
    {
      // Required time at which to register the source in seconds since the
      // UNIX epoch.
      "source_time": 123,

      // Required origin on which to register the source.
      "source_origin": "https://source.example",

      // Required source type, either "navigation" or "event", corresponding to
      // whether the source is registered on click or on view, respectively.
      "source_type": "navigation",

      "registration_config": {
        // Required uint64 formatted as a base-10 string.
        "source_event_id": "123456789",

        // Required site on which the source will be attributed.
        "destination": "https://destination.example",

        // Required origin to which the report will be sent if the source is
        // attributed.
        "reporting_origin": "https://reporting.example",

        // Optional int64 in milliseconds formatted as a base-10 string.
        // Defaults to 30 days.
        "expiry": "864000000",

        // Optional int64 formatted as a base-10 string.
        // Defaults to 0.
        "priority": "-456",

        // Optional dictionary of filters and corresponding values. Defaults
        // to empty.
        "filter_data": {
          "a": ["b", "c"],
          "d": []
        },

        // Optional uint64 formatted as a base-10 string. Defaults to null.
        "debug_key": "987",
      }
    },
    ...
  ],

  // List of zero or more triggers to register.
  "triggers": [
    {
      // Required time at which to register the trigger in seconds since the
      // UNIX epoch.
      "trigger_time": 123,

      // Required site on which the trigger is being registered.
      "destination": "https://destination.example",

      // Required origin to which the report will be sent.
      "reporting_origin": "https://reporting.example",

      "registration_config": {
        // Optional uint64 formatted as a base-10 string. Defaults to null.
        "debug_key": "987",

        // Optional dictionary of filters and corresponding values. Defaults
        // to empty.
        "filters": {
          "a": ["b", "c"],
          "d": []
        },

        "event_triggers": [
          {
            // Optional uint64 formatted as a base-10 string.
            // Defaults to 0.
            "trigger_data": "3",

            // Optional int64 formatted as a base-10 string.
            // Defaults to 0.
            "priority": "-456",

            // Optional int64 formatted as a base-10 string.
            // Defaults to null.
            "dedup_key": "789",

            // Optional dictionary of filters and corresponding values. Defaults
            // to empty.
            "filters": {
              "a": ["b", "c"],
              "d": []
            },

            // Optional dictionary of negated filters and corresponding values.
            // Defaults to empty.
            "not_filters": {
              "x": ["y"],
              "z": []
            }
          }
        ]
      }
    },
    ...
  ]
}

Output JSON format:

{
  // List of zero or more reports.
  "event_level_reports": [
    {
      // Time at which the report would have been sent in seconds since the
      // UNIX epoch.
      "report_time": 123,

      // URL to which the report would have been sent.
      "report_url": "https://reporting.example/.well-known/attribution-reporting/report-event-attribution",

      // The report itself. See
      // https://github.com/WICG/conversion-measurement-api/blob/main/EVENT.md#attribution-reports
      // for details about its fields.
      "report": { ... }
      },
    },
    ...
  ],
  // List of rejected sources. Omitted if empty.
  "rejected_sources": [
    {
      // Why the source was rejected.
      "reason": "excessiveReportingOrigins",

      // The original input JSON corresponding to the source.
      "source": { ... }
    }
  ],
  // List of rejected triggers. Omitted if empty.
  "rejected_triggers": [
    {
      // Why the trigger was rejected.
      "reason": "deduplicated",

      // The original input JSON corresponding to the trigger.
      "trigger": { ... }
    }
  ],
  // The original input, if the `copy_input_to_output` switch is present.
  "input": { ... }
}
)";

enum class InputMode { kSingle, kMulti };

void PrintHelp() {
  std::cerr << kHelpMsg;
}

int ProcessJsonString(const std::string& json_input,
                      const content::AttributionSimulationOptions& options,
                      bool copy_input_to_output,
                      int json_write_options) {
  base::JSONReader::ValueWithError result =
      base::JSONReader::ReadAndReturnValueWithError(
          json_input, base::JSONParserOptions::JSON_PARSE_RFC);
  if (!result.value) {
    std::cerr << "failed to deserialize input: " << result.error_message
              << std::endl;
    return 1;
  }

  base::Value input_copy;
  if (copy_input_to_output)
    input_copy = result.value->Clone();

  base::Value output = content::RunAttributionSimulation(
      std::move(*result.value), options, std::cerr);
  if (output.type() == base::Value::Type::NONE)
    return 1;

  if (copy_input_to_output)
    output.SetKey("input", std::move(input_copy));

  std::string output_json;
  bool success = base::JSONWriter::WriteWithOptions(output, json_write_options,
                                                    &output_json);
  if (!success) {
    std::cerr << "failed to serialize output JSON" << std::endl;
    return 1;
  }
  std::cout << output_json;
  return 0;
}

[[nodiscard]] bool ParseRandomizedResponseRateSwitch(
    const base::CommandLine& command_line,
    base::StringPiece switch_name,
    double* out) {
  if (!command_line.HasSwitch(switch_name))
    return true;

  std::string str = command_line.GetSwitchValueASCII(switch_name);
  double rate = 0;
  if (!base::StringToDouble(str, &rate)) {
    std::cerr << "invalid randomized response rate: " << str << std::endl;
    return false;
  }

  if (rate < 0 || rate > 1) {
    std::cerr << "randomized response rate must be between 0 and 1: " << rate
              << std::endl;
    return false;
  }

  *out = rate;
  return true;
}

// Required for setting up the test environment.
class TestSuite : public content::ContentTestSuiteBase {
 public:
  TestSuite(int argc, char** argv) : content::ContentTestSuiteBase(argc, argv) {
    Initialize();
    content::ForceInProcessNetworkService(true);
    // This initialization depends on the call to `Initialize()`, so we use a
    // `unique_ptr` to defer initialization instead of storing the field
    // directly.
    test_content_initializer_ =
        std::make_unique<content::TestContentClientInitializer>();
  }

 private:
  std::unique_ptr<content::TestContentClientInitializer>
      test_content_initializer_;
};

}  // namespace

int main(int argc, char* argv[]) {
  base::CommandLine::Init(argc, argv);
  const base::CommandLine& command_line =
      *base::CommandLine::ForCurrentProcess();

  if (!command_line.GetArgs().empty()) {
    std::cerr << "unexpected additional arguments" << std::endl;
    PrintHelp();
    return 1;
  }

  for (const auto& provided_switch : command_line.GetSwitches()) {
    if (!base::Contains(kAllowedSwitches, provided_switch.first)) {
      std::cerr << "unexpected switch `" << provided_switch.first << "`"
                << std::endl;
      PrintHelp();
      return 1;
    }
  }

  if (command_line.HasSwitch(kSwitchHelp) ||
      command_line.HasSwitch(kSwitchHelpShort)) {
    PrintHelp();
    return 0;
  }

  if (command_line.HasSwitch(kSwitchVersion) ||
      command_line.HasSwitch(kSwitchVersionShort)) {
    std::cout << version_info::GetVersionNumber() << std::endl;
    return 0;
  }

  auto noise_mode = content::AttributionNoiseMode::kDefault;
  if (command_line.HasSwitch(kSwitchNoiseMode)) {
    std::string str = command_line.GetSwitchValueASCII(kSwitchNoiseMode);

    if (str == "none") {
      noise_mode = content::AttributionNoiseMode::kNone;
    } else if (str != "default") {
      std::cerr << "unknown noise mode: " << str << std::endl;
      return 1;
    }
  }

  absl::optional<absl::uint128> noise_seed;
  if (command_line.HasSwitch(kSwitchNoiseSeed)) {
    if (noise_mode != content::AttributionNoiseMode::kDefault) {
      std::cerr << "noise seed may only be set when noise mode is `default`"
                << std::endl;
      return 1;
    }

    std::string str = command_line.GetSwitchValueASCII(kSwitchNoiseSeed);
    absl::uint128 value;
    if (!base::HexStringToUInt128(str, &value)) {
      std::cerr << "invalid noise seed: " << str << std::endl;
      return 1;
    }

    noise_seed = value;
  }

  auto randomized_response_rates =
      content::AttributionRandomizedResponseRates::kDefault;

  if (!ParseRandomizedResponseRateSwitch(
          command_line, kSwitchRandomizedResponseRateNavigation,
          &randomized_response_rates.navigation)) {
    return 1;
  }

  if (!ParseRandomizedResponseRateSwitch(command_line,
                                         kSwitchRandomizedResponseRateEvent,
                                         &randomized_response_rates.event)) {
    return 1;
  }

  auto delay_mode = content::AttributionDelayMode::kDefault;
  if (command_line.HasSwitch(kSwitchDelayMode)) {
    std::string str = command_line.GetSwitchValueASCII(kSwitchDelayMode);

    if (str == "none") {
      delay_mode = content::AttributionDelayMode::kNone;
    } else if (str != "default") {
      std::cerr << "unknown delay mode: " << str << std::endl;
      return 1;
    }
  }

  auto report_time_format =
      content::AttributionReportTimeFormat::kSecondsSinceUnixEpoch;
  if (command_line.HasSwitch(kSwitchReportTimeFormat)) {
    std::string str = command_line.GetSwitchValueASCII(kSwitchReportTimeFormat);

    if (str == "iso8601") {
      report_time_format = content::AttributionReportTimeFormat::kISO8601;
    } else if (str != "seconds_since_unix_epoch") {
      std::cerr << "unknown report time format: " << str << std::endl;
      return 1;
    }
  }

  auto input_mode = InputMode::kSingle;
  if (command_line.HasSwitch(kSwitchInputMode)) {
    std::string input_mode_string =
        command_line.GetSwitchValueASCII(kSwitchInputMode);
    if (input_mode_string == "multi") {
      input_mode = InputMode::kMulti;
    } else if (input_mode_string != "single") {
      std::cerr << "bad input_mode encountered: `" << input_mode_string << "`"
                << std::endl;
      PrintHelp();
      return 1;
    }
  }

  const bool copy_input_to_output =
      command_line.HasSwitch(kSwitchCopyInputToOutput);

  content::AttributionSimulationOptions options({
      .noise_mode = noise_mode,
      .noise_seed = noise_seed,
      .randomized_response_rates = randomized_response_rates,
      .delay_mode = delay_mode,
      .remove_report_ids = command_line.HasSwitch(kSwitchRemoveReportIds),
      .report_time_format = report_time_format,
      .remove_assembled_report =
          command_line.HasSwitch(kSwitchRemoveAssembledReport),
  });

  // Required for setting up the test environment.
  TestSuite test_suite(argc, argv);
  mojo::core::Init();

  switch (input_mode) {
    case InputMode::kSingle: {
      // Read all of stdin into a big string until a null char, as we don't have
      // a streaming JSON parser available.
      std::string input_string;
      std::getline(std::cin, input_string, '\0');
      return ProcessJsonString(input_string, options, copy_input_to_output,
                               base::JSONWriter::OPTIONS_PRETTY_PRINT);
    }
    case InputMode::kMulti: {
      int ret = 0;
      std::string line;
      while (std::getline(std::cin, line) && ret == 0) {
        ret = ProcessJsonString(line, options, copy_input_to_output, 0);
        std::cout << std::endl;
      }
      return ret;
    }
  }
}
