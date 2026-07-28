import { useState, useEffect } from 'react';
import { invoke } from '@tauri-apps/api/core';

const BASE_URL = 'https://knotlink.cn';

interface StorePlugin {
  id: string;
  name: string;
  type: string;
  typeLabel: string;
  typeIcon: string;
  dir: string;
  author: string;
  version: string;
  desc: string;
  appId: string;
  downloadUrl: string;
  logo: string | null;
  appName: string;
  socketsCount: number;
  signalsCount: number;
}

interface Props {
  plugin: StorePlugin;
  installed: boolean;
  onInstalled: () => void;
}

const StorePreview: React.FC<Props> = ({ plugin, installed, onInstalled }) => {
  const [readme, setReadme] = useState<string | null>(null);
  const [funcList, setFuncList] = useState<any>(null);
  const [loading, setLoading] = useState(true);
  const [installing, setInstalling] = useState(false);

  useEffect(() => {
    setReadme(null);
    setFuncList(null);
    setLoading(true);

    const readmeUrl = `${BASE_URL}/nodes/${plugin.dir}/README.md`;
    const funcUrl   = `${BASE_URL}/nodes/${plugin.dir}/FuncList.json`;

    Promise.all([
      invoke<string>('http_get_text', { url: readmeUrl }).catch(() => '# 暂无说明文档'),
      invoke<string>('http_get_text', { url: funcUrl })
        .then(t => { try { return JSON.parse(t); } catch { return null; } })
        .catch(() => null),
    ]).then(([r, f]) => {
      setReadme(r as string);
      setFuncList(f);
    }).catch(() => {
      setReadme('# 加载失败');
    }).finally(() => {
      setLoading(false);
    });
  }, [plugin.id]);

  const handleInstall = async () => {
    if (!plugin.downloadUrl) return;
    setInstalling(true);
    try {
      await invoke('download_and_install', { url: plugin.downloadUrl });
      onInstalled();
    } catch (err: any) {
      alert(`安装失败: ${err}`);
    } finally {
      setInstalling(false);
    }
  };

  const renderMarkdown = (md: string) => {
    const lines = md.split('\n');
    return lines.map((line, i) => {
      if (line.startsWith('### ')) return <h4 key={i}>{line.slice(4)}</h4>;
      if (line.startsWith('## ')) return <h3 key={i}>{line.slice(3)}</h3>;
      if (line.startsWith('# ')) return <h2 key={i}>{line.slice(2)}</h2>;
      if (line.startsWith('- **')) {
        const match = line.match(/- \*\*(.+?)\*\*(.*)/);
        if (match) return <p key={i}><strong>{match[1]}</strong>{match[2]}</p>;
      }
      if (line.startsWith('- ')) return <p key={i} style={{ paddingLeft: 16 }}>• {line.slice(2)}</p>;
      if (line.startsWith('```')) return null;
      if (line.trim() === '') return <br key={i} />;
      return <p key={i}>{line}</p>;
    });
  };

  return (
    <div className="node-preview">
      {/* 头部 */}
      <div style={{ display: 'flex', alignItems: 'center', gap: 12, marginBottom: 16 }}>
        {plugin.logo ? (
          <img src={`${BASE_URL}/${plugin.logo}`} alt={plugin.name}
            style={{ width: 48, height: 48, objectFit: 'contain', borderRadius: 8 }} />
        ) : (
          <span style={{ fontSize: 36 }}>{plugin.typeIcon}</span>
        )}
        <div>
          <div style={{ fontWeight: 600, fontSize: 16 }}>{plugin.name}</div>
          <div style={{ fontSize: 12, color: '#6c757d' }}>
            {plugin.author} · {plugin.version} · {plugin.typeLabel}
          </div>
        </div>
      </div>

      {/* 描述 */}
      {plugin.desc && (
        <div className="preview-field" style={{ marginBottom: 12 }}>{plugin.desc}</div>
      )}

      {/* 安装按钮 */}
      <div style={{ marginBottom: 16 }}>
        {installed ? (
          <button disabled style={{
            padding: '6px 20px', border: 'none', borderRadius: 6,
            background: '#d1fae5', color: '#065f46', fontSize: 13, fontWeight: 500,
          }}>
            ✅ 已安装
          </button>
        ) : installing ? (
          <button disabled style={{
            padding: '6px 20px', border: 'none', borderRadius: 6,
            background: '#e2e8f0', color: '#94a3b8', fontSize: 13,
          }}>
            ⏳ 安装中...
          </button>
        ) : (
          <button onClick={handleInstall} style={{
            padding: '6px 20px', border: 'none', borderRadius: 6,
            background: '#667eea', color: '#fff', fontSize: 13, fontWeight: 500,
            cursor: 'pointer',
          }}>
            ⬇️ 安装
          </button>
        )}
      </div>

      <hr />

      {loading ? (
        <div className="preview-loading">加载详情中...</div>
      ) : (
        <>
          {/* FuncList 接口 */}
          {funcList?.openSocket && Object.keys(funcList.openSocket).length > 0 && (
            <div style={{ marginBottom: 16 }}>
              <h4 style={{ fontSize: 14, margin: '12px 0 8px' }}>⚡ 功能接口</h4>
              {Object.entries(funcList.openSocket).map(([key, val]: [string, any]) => (
                <div key={key} className="preview-field" style={{ marginBottom: 8 }}>
                  <strong>{key}</strong>{' '}
                  <code style={{ fontSize: 11, background: '#f1f3f5', padding: '1px 6px', borderRadius: 3 }}>
                    {val.openSocketID}
                  </code>
                  {val.description && <div style={{ fontSize: 12, color: '#6c757d', marginTop: 2 }}>{val.description}</div>}
                  {val.args && Object.keys(val.args).length > 0 && (
                    <div style={{ fontSize: 11, color: '#94a3b8', marginTop: 2 }}>
                      参数: {Object.keys(val.args).join(', ')}
                    </div>
                  )}
                </div>
              ))}
            </div>
          )}

          {/* Signal */}
          {funcList?.signal && Object.keys(funcList.signal).length > 0 && (
            <div style={{ marginBottom: 16 }}>
              <h4 style={{ fontSize: 14, margin: '12px 0 8px' }}>📢 信号</h4>
              {Object.entries(funcList.signal).map(([key, val]: [string, any]) => (
                <div key={key} className="preview-field" style={{ marginBottom: 6, fontSize: 13 }}>
                  <strong>{key}</strong> ({val.signalID}): {val.description}
                </div>
              ))}
            </div>
          )}

          {/* README */}
          {readme && readme !== '# 暂无说明文档' && (
            <div>
              <h4 style={{ fontSize: 14, margin: '12px 0 8px' }}>📖 说明</h4>
              <div style={{ fontSize: 13, lineHeight: 1.6 }}>
                {renderMarkdown(readme)}
              </div>
            </div>
          )}
        </>
      )}
    </div>
  );
};

export default StorePreview;
