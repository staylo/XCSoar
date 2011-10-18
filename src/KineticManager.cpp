/*
 * Copyright (C) 2011 Tobias Bieniek <Tobias.Bieniek@gmx.de>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the
 * distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * FOUNDATION OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "KineticManager.hpp"
#include <cstdio>

void
KineticManager::MouseDown(int x)
{
  steady = false;
  last = x;
  clock.update();
  printf("down - %06d\n", x);
}

void
KineticManager::MouseMove(int x)
{
  // Get time since last position update
  int dt = clock.elapsed();

  // Filter fast updates to get a better velocity
  if (dt < 15)
    return;

  // Update clock for next event
  clock.update();

  // Calculate value delta
  int dx = x - last;

  // Calculate value-based velocity
  v = fixed(dx) / dt;

  // Save value for next event
  last = x;
  printf("move - %06d (dx: %+03d - dt: %04d - v: %02.3f)\n", x, dx, dt, (double)v);
}

void
KineticManager::MouseUp(int x)
{
  MouseMove(x);

  end = last + (int)((v / 2) * stopping_time);

  printf("up   - %06d (end: %06d)\n", x, end);
}

int
KineticManager::GetPosition()
{
  int t = clock.elapsed();

  if (t >= stopping_time) {
    steady = true;
    return end;
  }

  int x = last + (int)(v * t - v * t * t / (2 * stopping_time));
  if (x == end)
    steady = true;

  printf("pos  - %06d (end: %06d)\n", x, end);
  return x;
}

bool
KineticManager::IsSteady()
{
  return steady;
}
