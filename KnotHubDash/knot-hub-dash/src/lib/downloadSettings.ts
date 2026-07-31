export type DownloadMode = 'direct' | 'mirror' | 'ask';

export interface DownloadSettings {
  mode: DownloadMode;
  mirrorUrl: string;
  useMultiDownload: boolean;
}

// 预设镜像列表
export const MIRROR_PRESETS = [
  { label: 'gh-proxy.com',     url: 'https://gh-proxy.com/' },
  { label: 'gh.con.sh',        url: 'https://gh.con.sh/' },
  { label: 'ghproxy.cc',       url: 'https://ghproxy.cc/' },
];

const STORAGE_KEY = 'knothub_download_settings';
const DEFAULT_MIRROR = MIRROR_PRESETS[0].url;

export function getSettings(): DownloadSettings {
  try {
    const raw = localStorage.getItem(STORAGE_KEY);
    if (raw) return JSON.parse(raw);
  } catch {}
  return { mode: 'ask', mirrorUrl: DEFAULT_MIRROR, useMultiDownload: false };
}

export function saveSettings(s: DownloadSettings) {
  localStorage.setItem(STORAGE_KEY, JSON.stringify(s));
}
