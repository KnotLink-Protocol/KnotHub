import { Channel } from '@tauri-apps/api/core';
import { invoke } from '@tauri-apps/api/core';
import { openUrl } from '@tauri-apps/plugin-opener';
import { getSettings, MIRROR_PRESETS } from './downloadSettings';
import { startDownload, updateProgress, finishDownload } from './downloadState';

export async function downloadWithRetry(url: string): Promise<void> {
  const settings = getSettings();

  const doDownload = async (url: string, mirrorUrl?: string) => {
    const channel = new Channel<{ downloaded: number; total: number | null }>((msg) => {
      updateProgress(msg.downloaded, msg.total);
    });
    await invoke('download_and_install', {
      url, mirrorUrl,
      onProgress: channel,
      useMd: settings.useMultiDownload,
    });
  };

  startDownload();
  try {
    // 模式：总是镜像 → 依次尝试所有预设镜像
    if (settings.mode === 'mirror') {
      const mirrors = settings.mirrorUrl
        ? [settings.mirrorUrl, ...MIRROR_PRESETS.map(p => p.url).filter(u => u !== settings.mirrorUrl)]
        : MIRROR_PRESETS.map(p => p.url);
      for (const m of mirrors) {
        try {
          await doDownload(url, m);
          return; // 成功
        } catch (_err: any) {
          // 静默换下一个镜像
        }
      }
      throw new Error('所有镜像均无法访问，建议开启 VPN/代理后重试');
    }

    // 模式：直连
    if (settings.mode === 'direct') {
      await doDownload(url);
      return;
    }

    // 模式：询问 → 失败后弹窗，依次尝试镜像
    try {
      await doDownload(url);
    } catch (err) {
      const useMirror = confirm(
        `下载失败，可能是 GitHub 无法访问。\n\n` +
        `错误: ${err}\n\n` +
        `是否使用镜像加速重试？`
      );
      if (!useMirror) {
        if (confirm('是否在浏览器中打开下载页面？')) await openUrl(url);
        throw err;
      }

      // 依次尝试预设镜像
      let mirrorErr: string = '';
      for (const m of MIRROR_PRESETS.map(p => p.url)) {
        try {
          await doDownload(url, m);
          return; // 成功
        } catch (err: any) {
          mirrorErr = String(err);
          // 如果是镜像本身的问题（HTML 返回），静默换下一个
          if (mirrorErr.includes('网页而非文件') || mirrorErr.includes('text/html')) {
            continue;
          }
          // 其他错误（网络、文件问题）也继续尝试
        }
      }
      throw new Error(
        `所有镜像均无法访问（已尝试 ${MIRROR_PRESETS.length} 个镜像站）。\n` +
        `建议：开启 VPN/代理后重试，或手动下载 zip 拖入插件页面安装。`
      );
    }
  } finally {
    finishDownload();
  }
}
