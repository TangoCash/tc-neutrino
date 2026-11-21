--[[
	pbs.lua - HLS test streams from https://github.com/pbs/test-streams
	arg[1]: stream key (see table below)
]]

if #arg < 1 then return nil end

json = require "json"

local WVD = "/var/tuxbox/config/device.wvd"
local AXINOM_TOKEN = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.ewogICJ2ZXJzaW9uIjogMSwKICAiY29tX2tleV9pZCI6ICI2OWU1NDA4OC1lOWUwLTQ1MzAtOGMxYS0xZWI2ZGNkMGQxNGUiLAogICJtZXNzYWdlIjogewogICAgInR5cGUiOiAiZW50aXRsZW1lbnRfbWVzc2FnZSIsCiAgICAidmVyc2lvbiI6IDIsCiAgICAibGljZW5zZSI6IHsKICAgICAgImFsbG93X3BlcnNpc3RlbmNlIjogdHJ1ZQogICAgfSwKICAgICJjb250ZW50X2tleXNfc291cmNlIjogewogICAgICAiaW5saW5lIjogWwogICAgICAgIHsKICAgICAgICAgICJpZCI6ICIzMDJmODBkZC00MTFlLTQ4ODYtYmNhNS1iYjFmODAxOGEwMjQiLAogICAgICAgICAgImVuY3J5cHRlZF9rZXkiOiAicm9LQWcwdDdKaTFpNDNmd3YremZ0UT09IiwKICAgICAgICAgICJ1c2FnZV9wb2xpY3kiOiAiUG9saWN5IEEiCiAgICAgICAgfQogICAgICBdCiAgICB9LAogICAgImNvbnRlbnRfa2V5X3VzYWdlX3BvbGljaWVzIjogWwogICAgICB7CiAgICAgICAgIm5hbWUiOiAiUG9saWN5IEEiLAogICAgICAgICJwbGF5cmVhZHkiOiB7CiAgICAgICAgICAibWluX2RldmljZV9zZWN1cml0eV9sZXZlbCI6IDE1MCwKICAgICAgICAgICJwbGF5X2VuYWJsZXJzIjogWwogICAgICAgICAgICAiNzg2NjI3RDgtQzJBNi00NEJFLThGODgtMDhBRTI1NUIwMUE3IgogICAgICAgICAgXQogICAgICAgIH0KICAgICAgfQogICAgXQogIH0KfQ._NfhLVY7S6k8TJDWPeMPhUawhympnrk6WAZHOVjER6M"

local drm = {
	shaka  = { license_url = "https://cwip-shaka-proxy.appspot.com/no_auth" },
	axinom = { license_url = "https://drm-widevine-licensing.axprod.net/AcquireLicense",
	           headers = { { ["X-AxDRM-Message"] = AXINOM_TOKEN } } },
}

local streams = {
	mux_bbb = { url = "https://pbs.github.io/test-streams/test-streams.mux.dev/x36xhzz/x36xhzz.m3u8", drm = nil },
	bars_hevc_avc = { url = "https://pbs.github.io/test-streams/pbs/test-pattern/pbs-bars_hevc-avc.m3u8", drm = nil },
	bars_cmaf_iframes = { url = "https://pbs.github.io/test-streams/pbs/test-pattern-dedicated-iframes/pbs-bars_avc.m3u8", drm = nil },
	muxed_avc = { url = "https://pbs.github.io/test-streams/pbs/test-pattern-muxed-fmp4/pbs-bars_muxed-avc.m3u8", drm = nil },
	muxed_hevc = { url = "https://pbs.github.io/test-streams/pbs/test-pattern-muxed-fmp4/pbs-bars_muxed-hevc.m3u8", drm = nil },
	muxed_hevc_apple = { url = "https://pbs.github.io/test-streams/pbs/test-pattern-muxed-fmp4/pbs-bars_muxed-hevc-apple.m3u8", drm = nil },
	angel_one_wv = { url = "https://pbs.github.io/test-streams/storage.googleapis.com/shaka-demo-assets/angel-one-widevine-hls/hls.m3u8", drm = "shaka" },
	muxed_avc_cbcs = { url = "https://pbs.github.io/test-streams/pbs/test-pattern-muxed-fmp4/pbs-bars_muxed-avc-cbcs.m3u8", drm = "axinom" },
	muxed_avc_apple_cbcs = { url = "https://pbs.github.io/test-streams/pbs/test-pattern-muxed-fmp4/pbs-bars_muxed-avc-apple-cbcs.m3u8", drm = "axinom" },
	drm_avc = { url = "https://pbs.github.io/test-streams/pbs/test-pattern-drm/pbs-bars_avc.m3u8", drm = "axinom" },
	drm_hevc_avc = { url = "https://pbs.github.io/test-streams/pbs/test-pattern-drm/pbs-bars_hevc-avc.m3u8", drm = "axinom" },
	drm4k_hevc_avc = { url = "https://pbs.github.io/test-streams/pbs/4k-drm-dedicated-iframes/pbs-bars_hevc-avc.m3u8", drm = "axinom" },
}

local s = streams[arg[1]]
if s == nil then return nil end

local entry = {}
entry['url']  = s.url
entry['band'] = "1"
entry['res1'] = "1"
entry['res2'] = "1"
entry['name'] = "PBS " .. arg[1]

if s.drm then
	local d = drm[s.drm]
	local api = {}
	api['license_url'] = d.license_url
	api['wv_device'] = WVD
	if d.headers then api['license_headers'] = d.headers end
	entry['drmhandler_parameters'] = api
end

local ret = {}
ret[1] = entry
return json:encode(ret)
