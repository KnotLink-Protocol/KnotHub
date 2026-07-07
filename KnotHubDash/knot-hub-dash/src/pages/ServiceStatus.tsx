import { useState, useEffect, useCallback } from 'react';
import { invoke } from '@tauri-apps/api/core';

interface PortStatus {
  port: string;
  service: string;
  desc: string;
  status: 'checking' | 'online' | 'offline';
}

const PORTS: PortStatus[] = [
  { port: '6376',    service: 'KnotLink',       desc: 'KnotLink TCP 通信服务' },
];

export default function ServiceStatus() {
  const [ports, setPorts] = useState<PortStatus[]>(PORTS);
  const [polling, setPolling] = useState(true);

  const checkAll = useCallback(async () => {
    const updated = await Promise.all(
      ports.map(async (p) => {
        try {
          const ok = await invoke<boolean>('check_service_port', { addr: `127.0.0.1:${p.port}` });
          return { ...p, status: ok ? 'online' as const : 'offline' as const };
        } catch {
          return { ...p, status: 'offline' as const };
        }
      })
    );
    setPorts(updated);
  }, []);

  useEffect(() => {
    checkAll();
    if (!polling) return;
    const timer = setInterval(checkAll, 5000);
    return () => clearInterval(timer);
  }, [checkAll, polling]);

  const handleRowClick = (p: PortStatus) => {
    window.dispatchEvent(new CustomEvent('update-preview', {
      detail: { type: 'port', id: p.port, details: p },
    }));
  };

  const onlineCount = ports.filter(p => p.status === 'online').length;

  return (
    <>
      <div style={{ marginBottom: 16 }}>
        <h1 style={{ fontSize: 22, fontWeight: 500 }}>服务状态</h1>
        <p style={{ color: '#6c757d' }}>
          端口监控 · {onlineCount}/{ports.length} 在线
        </p>
      </div>

      <div style={{ marginBottom: 12, display: 'flex', gap: 8, alignItems: 'center' }}>
        <button className="btn btn-sm" onClick={checkAll}>🔄 立即检测</button>
        <label style={{ fontSize: 13, color: '#6c757d', cursor: 'pointer' }}>
          <input
            type="checkbox"
            checked={polling}
            onChange={(e) => setPolling(e.target.checked)}
            style={{ marginRight: 4 }}
          />
          每 5 秒自动检测
        </label>
      </div>

      <div className="section">
        <table className="monitor-table">
          <thead>
            <tr><th>端口</th><th>服务</th><th>说明</th><th>状态</th></tr>
          </thead>
          <tbody>
            {ports.map(p => (
              <tr
                key={p.port}
                onClick={() => handleRowClick(p)}
                style={{ cursor: 'pointer' }}
              >
                <td><code>{p.port}</code></td>
                <td>{p.service}</td>
                <td style={{ color: '#6c757d', fontSize: 13 }}>{p.desc}</td>
                <td>
                  <span className={
                    p.status === 'online'  ? 'status-badge' :
                    p.status === 'offline' ? 'status-badge warning' :
                    'status-badge'
                  }>
                    {p.status === 'online' ? '在线' : p.status === 'offline' ? '离线' : '检测中'}
                  </span>
                </td>
              </tr>
            ))}
          </tbody>
        </table>
      </div>
    </>
  );
}
