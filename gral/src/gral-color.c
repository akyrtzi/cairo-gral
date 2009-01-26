/* Copyright (c) 2009, Argiris Kirtzidis
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY ARGIRIS KIRTZIDIS ''AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL ARGIRIS KIRTZIDIS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "gral-internal.h"
#include "gral.h"

static gral_color_t const gral_color_white = {
  1.0, 1.0, 1.0, 1.0
};

static gral_color_t const gral_color_black = {
  0.0, 0.0, 0.0, 1.0
};

static gral_color_t const gral_color_zero = {
  0.0, 0.0, 0.0, 0.0
};

static gral_color_t const gral_color_magenta = {
  1.0, 0.0, 1.0, 1.0
};

const gral_color_t *
gral_stock_color (gral_stock_t stock)
{
  switch (stock) {
    case GRAL_STOCK_WHITE:
      return &gral_color_white;
    case GRAL_STOCK_BLACK:
      return &gral_color_black;
    case GRAL_STOCK_ZERO:
      return &gral_color_zero;
  }

  ASSERT_NOT_REACHED;

  /* If the user can get here somehow, give a color that indicates a
  * problem. */
  return &gral_color_magenta;
}
