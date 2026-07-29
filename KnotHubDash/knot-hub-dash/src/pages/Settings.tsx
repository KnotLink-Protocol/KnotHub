import { useState, useEffect } from 'react';
import { invoke } from '@tauri-apps/api/core';
import { openUrl } from '@tauri-apps/plugin-opener';

interface UpdateInfo {
  current: string;
  latest: string;
  has_update: boolean;
  published_at: string | null;
  html_url: string | null;
  body: string | null;
}

export default function Settings() {
  const [autostart, setAutostart] = useState(false);
  const [checking, setChecking] = useState(true);
  const [update, setUpdate] = useState<UpdateInfo | null>(null);
  const [checkingUpdate, setCheckingUpdate] = useState(false);

  const knotlinkPorts = [
    { port: '6378', service: 'OpenSocket', label: '回答者注册' },
    { port: '6376', service: 'OpenSocket', label: '查询请求' },
    { port: '6372', service: 'Signal',     label: '订阅注册' },
    { port: '6370', service: 'Signal',     label: '信号发送' },
  ];
  const [portStatus, setPortStatus] = useState<Record<string, boolean>>({});

  const checkUpdate = async () => {
    setCheckingUpdate(true);
    try {
      const u = await invoke<UpdateInfo>('check_latest_version');
      setUpdate(u);
    } catch {
      setUpdate(prev => prev || null);
    } finally {
      setCheckingUpdate(false);
    }
  };

  useEffect(() => {
    (async () => {
      try {
        const a = await invoke<boolean>('get_core_autostart');
        setAutostart(a);

        const results: Record<string, boolean> = {};
        await Promise.all(knotlinkPorts.map(async ({ port }) => {
          try {
            results[port] = await invoke<boolean>('check_service_port', { addr: `127.0.0.1:${port}` });
          } catch {
            results[port] = false;
          }
        }));
        setPortStatus(results);
      } catch (err) {
        console.error('加载设置失败:', err);
      } finally {
        setChecking(false);
      }
    })();
    checkUpdate();
  }, []);

  const toggleAutostart = async () => {
    try {
      await invoke('set_core_autostart', { enable: !autostart });
      setAutostart(!autostart);
    } catch (err) {
      alert(`设置失败: ${err}`);
    }
  };

  return (
    <>
      <div style={{ marginBottom: 16 }}>
        <h1 style={{ fontSize: 22, fontWeight: 500 }}>设置</h1>
        <p style={{ color: '#6c757d' }}>系统配置与状态</p>
      </div>

      {/* 系统服务 */}
      <div className="section" style={{ marginBottom: 16 }}>
        <div className="section-header">
          <div className="section-title">系统服务</div>
        </div>
        <div style={{ display: 'flex', flexDirection: 'column', gap: 12 }}>
          {knotlinkPorts.map(({ port, service, label }) => (
            <div key={port} style={{ display: 'flex', alignItems: 'center', justifyContent: 'space-between' }}>
              <div>
                <div style={{ fontWeight: 500 }}>KnotLink {service}</div>
                <div style={{ color: '#6c757d', fontSize: 13 }}>{label} · 端口 {port}</div>
              </div>
              <span className={portStatus[port] ? 'status-badge' : 'status-badge warning'}>
                {portStatus[port] === undefined ? '...' : portStatus[port] ? '在线' : '离线'}
              </span>
            </div>
          ))}

          <div style={{ display: 'flex', alignItems: 'center', justifyContent: 'space-between' }}>
            <div>
              <div style={{ fontWeight: 500 }}>KnotHubCore 开机自启</div>
              <div style={{ color: '#6c757d', fontSize: 13 }}>
                {autostart ? '登录后自动启动守护进程' : '需手动启动'}
              </div>
            </div>
            <button
              className="btn btn-sm"
              onClick={toggleAutostart}
              disabled={checking}
              style={{
                background: autostart ? '#667eea' : '#ddd',
                color: autostart ? '#fff' : '#666',
                border: 'none',
                borderRadius: 14,
                padding: '4px 16px',
                cursor: 'pointer',
                fontWeight: 500,
              }}
            >
              {checking ? '...' : autostart ? '已开启 ✓' : '已关闭 ✗'}
            </button>
          </div>
        </div>
      </div>

      {/* 数据路径 */}
      <div className="section" style={{ marginBottom: 16 }}>
        <div className="section-header">
          <div className="section-title">数据路径</div>
        </div>
        <div style={{ color: '#6c757d', fontSize: 13, lineHeight: 2 }}>
          <div>插件目录: <code style={{ background: '#f0f0f0', padding: '1px 6px', borderRadius: 3 }}>Plugins\</code></div>
          <div>配方目录: <code style={{ background: '#f0f0f0', padding: '1px 6px', borderRadius: 3 }}>Recipes\</code></div>
          <div style={{ marginTop: 4, fontSize: 12 }}>
            均位于 KnotHubCore.exe 所在目录下
          </div>
        </div>
      </div>

      {/* 关于 */}
      <div className="section">
        <div className="section-header">
          <div className="section-title">关于</div>
        </div>
        <div style={{ color: '#6c757d', fontSize: 13, lineHeight: 2 }}>
          <div style={{ display: 'flex', alignItems: 'center', gap: 12 }}>
            <span>KnotHub v{update?.current || '...'}</span>
            <button className="btn btn-sm"
              onClick={checkUpdate} disabled={checkingUpdate}
              style={{ border: '1px solid var(--border)', borderRadius: 6, cursor: 'pointer' }}>
              {checkingUpdate ? '...' : '🔄 检查更新'}
            </button>
          </div>

          {update && (
            update.has_update ? (
              <div style={{
                marginTop: 8, padding: '8px 12px',
                background: '#fef2c7', borderRadius: 6, color: '#92400e',
                display: 'flex', alignItems: 'center', gap: 8,
              }}>
                <span>🆕 {update.latest} 可用</span>
                {update.published_at && (
                  <span style={{ fontSize: 11, opacity: 0.7 }}>
                    {new Date(update.published_at).toLocaleDateString()}
                  </span>
                )}
                <button className="btn btn-sm"
                  onClick={() => update.html_url && openUrl(update.html_url)}
                  style={{ marginLeft: 'auto', border: '1px solid #92400e', borderRadius: 6, cursor: 'pointer', background: 'none' }}>
                  📦 下载
                </button>
              </div>
            ) : (
              <div style={{ marginTop: 4, color: '#065f46' }}>✅ 已是最新版本</div>
            )
          )}

          <div style={{ marginTop: 8 }}>KnotLink 协议服务中枢 &amp; 管理面板</div>
          <div style={{ marginTop: 4 }}>
            <a href="https://github.com/KnotLink-Protocol/KnotHub" target="_blank" rel="noreferrer"
               style={{ color: '#667eea' }}>
              GitHub → KnotLink-Protocol/KnotHub
            </a>
          </div>
        </div>
      </div>
    </>
  );
}
