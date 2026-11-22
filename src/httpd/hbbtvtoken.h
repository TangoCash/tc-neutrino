/*
 * hbbtvtoken.h - the credential of the HbbTV browser on the box
 *
 * Copyright (C) 2026 NI-Team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef __httpd_hbbtvtoken_h__
#define __httpd_hbbtvtoken_h__

#include <string>

namespace httpd
{

/* A token for the HbbTV browser the box starts: it drives the media of its
   pages through /api/v1/hbbtv and nothing else. Scoped and in memory only,
   like every scoped token here; empty when none could be drawn. */
std::string openHbbtvToken();

} // namespace httpd

#endif
