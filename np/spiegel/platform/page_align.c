/*
 * Copyright 2011-2020 Gregory Banks
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * This file compiles to a .text segment containing zero bytes but
 * aligned to a page size.  It's useful for the hack which ensures that
 * NovaProva's copy of the mprotect() system call stub is on it's own
 * page and cannot accidentally render itself unexecutable.
 */

__asm__(".align 4096");

