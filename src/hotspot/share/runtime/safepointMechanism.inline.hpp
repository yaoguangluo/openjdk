/*
 * Copyright (c) 2017, 2019, Oracle and/or its affiliates. All rights reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 * This code is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 only, as
 * published by the Free Software Foundation.
 *
 * This code is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * version 2 for more details (a copy is included in the LICENSE file that
 * accompanied this code).
 *
 * You should have received a copy of the GNU General Public License version
 * 2 along with this work; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Please contact Oracle, 500 Oracle Parkway, Redwood Shores, CA 94065 USA
 * or visit www.oracle.com if you need additional information or have any
 * questions.
 *
 */

#ifndef SHARE_RUNTIME_SAFEPOINTMECHANISM_INLINE_HPP
#define SHARE_RUNTIME_SAFEPOINTMECHANISM_INLINE_HPP

#include "runtime/safepointMechanism.hpp"
#include "runtime/safepoint.hpp"
#include "runtime/thread.inline.hpp"

bool SafepointMechanism::local_poll_armed(JavaThread* thread) {
  const intptr_t poll_word = reinterpret_cast<intptr_t>(thread->get_polling_page());
  return mask_bits_are_true(poll_word, poll_bit());
}

bool SafepointMechanism::global_poll() {
  return SafepointSynchronize::do_call_back();
}

bool SafepointMechanism::local_poll(Thread* thread) {
  if (thread->is_Java_thread()) {
    return local_poll_armed((JavaThread*)thread);
  } else {
    // If the poll is on a non-java thread we can only check the global state.
    return global_poll();
  }
}

bool SafepointMechanism::should_block(Thread* thread) {
  if (uses_thread_local_poll()) {
    return local_poll(thread);
  } else {
    return global_poll();
  }
}

void SafepointMechanism::block_if_requested(JavaThread *thread) {
  if (uses_thread_local_poll() && !SafepointMechanism::local_poll_armed(thread)) {
    return;
  }
  block_if_requested_slow(thread);
}

void SafepointMechanism::callback_if_safepoint(JavaThread* thread) {
  if (!uses_thread_local_poll() || local_poll_armed(thread)) {
    // If using thread local polls, we should not check the
    // global_poll() and callback via block() if the VMThread
    // has not yet armed the local poll. Otherwise, when used in
    // combination with should_block(), the latter could miss
    // detecting the same safepoint that this method would detect
    // if only checking global polls.
    if (global_poll()) {
      SafepointSynchronize::block(thread, false);
    }
  }
}

void SafepointMechanism::arm_local_poll(JavaThread* thread) {
  thread->set_polling_page(poll_armed_value());
}

void SafepointMechanism::disarm_local_poll(JavaThread* thread) {
  thread->set_polling_page(poll_disarmed_value());
}

void SafepointMechanism::arm_local_poll_release(JavaThread* thread) {
  thread->set_polling_page_release(poll_armed_value());
}

void SafepointMechanism::disarm_local_poll_release(JavaThread* thread) {
  thread->set_polling_page_release(poll_disarmed_value());
}

#endif // SHARE_RUNTIME_SAFEPOINTMECHANISM_INLINE_HPP
