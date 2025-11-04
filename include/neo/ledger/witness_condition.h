/**
 * @file witness_condition.h
 * @brief Backwards-compatible include for witness rule conditions.
 *
 * This header exists to preserve legacy include paths that referenced
 * `neo/ledger/witness_condition.h` in the original C# port.
 * The condition hierarchy now lives in witness_rule.h; include that directly
 * for new code.
 */

#pragma once

#include <neo/ledger/witness_rule.h>

