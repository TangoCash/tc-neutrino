--[[
	axinom.lua - Axinom public test vectors v10 (Widevine), for DRM path tests
	arg[1]: "cmaf" (H264 single-key, cbcs) or "dash" (H264 single-key, cenc)
	source: https://github.com/Axinom/public-test-vectors
]]

if #arg < 1 then return nil end

json = require "json"

local vectors = {
	cmaf = {
		url   = "https://media.axprod.net/TestVectors/Cmaf/protected_1080p_h264_cbcs/manifest.mpd",
		token = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJ2ZXJzaW9uIjogMSwKImNvbV9rZXlfaWQiOiAiZjM0ZjgzZWMtZTQyNS00OTYzLWExODMtYjRkNDAwYmNlNDQ0IiwKIm1lc3NhZ2UiOiB7CiAgInR5cGUiOiAiZW50aXRsZW1lbnRfbWVzc2FnZSIsCiAgInZlcnNpb24iOiAyLAogICJsaWNlbnNlIjogewogICAgImFsbG93X3BlcnNpc3RlbmNlIjogdHJ1ZQogIH0sCiAgImNvbnRlbnRfa2V5c19zb3VyY2UiOiB7CiAgICAiaW5saW5lIjogWwogICAgICB7CiAgICAgICAgImlkIjogIjMwMmY4MGRkLTQxMWUtNDg4Ni1iY2E1LWJiMWY4MDE4YTAyNCIsCiAgICAgICAgImVuY3J5cHRlZF9rZXkiOiAiMWVYeWhGWmNGVjljemNtZG5BREpMdz09IiwKICAgICAgICAiaXYiOiAiZC8wWWlhcjBGRHNJVlVpendQbGJtZz09IiwKICAgICAgICAidXNhZ2VfcG9saWN5IjogIlBvbGljeSBBIgogICAgICB9CiAgICBdCiAgfSwKICAiY29udGVudF9rZXlfdXNhZ2VfcG9saWNpZXMiOiBbCiAgICB7CiAgICAgICJuYW1lIjogIlBvbGljeSBBIiwKICAgICAgInBsYXlyZWFkeSI6IHsKICAgICAgICAibWluX2RldmljZV9zZWN1cml0eV9sZXZlbCI6IDE1MCwKICAgICAgICAicGxheV9lbmFibGVycyI6IFsKICAgICAgICAgICI3ODY2MjdEOC1DMkE2LTQ0QkUtOEY4OC0wOEFFMjU1QjAxQTciCiAgICAgICAgXQogICAgICB9CiAgICB9CiAgXQp9fQ.5VxBJQsgGVissi9WCNpT7IoykEAmxaEcif6F3SeLvzU",
	},
	dash = {
		url   = "https://media.axprod.net/TestVectors/Dash/protected_dash_1080p_h264_singlekey/manifest.mpd",
		token = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJ2ZXJzaW9uIjogMSwKImNvbV9rZXlfaWQiOiAiZjM0ZjgzZWMtZTQyNS00OTYzLWExODMtYjRkNDAwYmNlNDQ0IiwKIm1lc3NhZ2UiOiB7CiAgInR5cGUiOiAiZW50aXRsZW1lbnRfbWVzc2FnZSIsCiAgInZlcnNpb24iOiAyLAogICJsaWNlbnNlIjogewogICAgImFsbG93X3BlcnNpc3RlbmNlIjogdHJ1ZQogIH0sCiAgImNvbnRlbnRfa2V5c19zb3VyY2UiOiB7CiAgICAiaW5saW5lIjogWwogICAgICB7CiAgICAgICAgImlkIjogIjQwNjBhODY1LTg4NzgtNDI2Ny05Y2JmLTkxYWU1YmFlMWU3MiIsCiAgICAgICAgImVuY3J5cHRlZF9rZXkiOiAieE5LNVNvRzRseGdHbGpRY2ZMOVRXQT09IiwKICAgICAgICAiaXYiOiAiTkJOYUpPcGI3MnNTYzBkSXFJejJQdz09IiwKICAgICAgICAidXNhZ2VfcG9saWN5IjogIlBvbGljeSBBIgogICAgICB9CiAgICBdCiAgfSwKICAiY29udGVudF9rZXlfdXNhZ2VfcG9saWNpZXMiOiBbCiAgICB7CiAgICAgICJuYW1lIjogIlBvbGljeSBBIiwKICAgICAgInBsYXlyZWFkeSI6IHsKICAgICAgICAibWluX2RldmljZV9zZWN1cml0eV9sZXZlbCI6IDE1MCwKICAgICAgICAicGxheV9lbmFibGVycyI6IFsKICAgICAgICAgICI3ODY2MjdEOC1DMkE2LTQ0QkUtOEY4OC0wOEFFMjU1QjAxQTciCiAgICAgICAgXQogICAgICB9CiAgICB9CiAgXQp9fQ.3Ti5rkzgw77GOXIpZWqNBUX16DXvph_NiJPCejrDStA",
	},
}

local v = vectors[arg[1]]
if v == nil then return nil end

local entry = {}
entry['url']  = v.url
entry['band'] = "1"
entry['res1'] = "1"
entry['res2'] = "1"
entry['name'] = "Axinom " .. arg[1]

local api = {}
api['license_url'] = "https://drm-widevine-licensing.axprod.net/AcquireLicense"
api['wv_device'] = "/var/tuxbox/config/device.wvd"
local hdrs = {}
hdrs[1] = { ['X-AxDRM-Message'] = v.token }
api['license_headers'] = hdrs
entry['drmhandler_parameters'] = api

local ret = {}
ret[1] = entry
return json:encode(ret)
