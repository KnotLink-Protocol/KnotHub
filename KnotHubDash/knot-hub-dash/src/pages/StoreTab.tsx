import { useState, useEffect } from 'react';
import { invoke } from '@tauri-apps/api/core';
import styles from './StoreTab.module.css';

// ── 数据结构（与 nodes-index.json 对齐）──────────────────

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
  specVersion: string;
  manifestVersion: string;
  socketsCount: number;
  signalsCount: number;
}

interface LocalPlugin {
  app_id: string;
  version?: string;
  name?: string;
}

// ── 网站地址 ─────────────────────────────────────────────

const BASE_URL = 'https://knotlink.cn';

// ── 导出给 Nodes.tsx ─────────────────────────────────────

export interface StoreTabRef {
  storePlugins: StorePlugin[];
  storeLoading: boolean;
  storeError: string | null;
}

interface Props {
  installedAppIds: Map<string, string>;   // app_id → 本地安装的版本
  onInstalledChange: () => void;           // 安装成功后通知父组件刷新
}

export default function StoreTab({ installedAppIds, onInstalledChange }: Props) {
  const [plugins, setPlugins] = useState<StorePlugin[]>([]);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState<string | null>(null);
  const [installingId, setInstallingId] = useState<string | null>(null);
  const [detail, setDetail] = useState<StorePlugin | null>(null);
  const [detailReadme, setDetailReadme] = useState<string | null>(null);
  const [detailFuncList, setDetailFuncList] = useState<any>(null);
  const [detailLoading, setDetailLoading] = useState(false);
  const [typeFilter, setTypeFilter] = useState<'all' | 'plugin' | 'standalone'>('all');
  const [searchQuery, setSearchQuery] = useState('');

  // ── 拉取商店索引 ──────────────────────────────────────

  const fetchIndex = async () => {
    setLoading(true);
    setError(null);
    try {
      const data = await invoke<StorePlugin[]>('fetch_store_index', {
        url: `${BASE_URL}/nodes-index.json`,
      });
      setPlugins(data);
    } catch (err: any) {
      setError(typeof err === 'string' ? err : (err?.message || String(err)));
      // 尝试本地 fallback
      try {
        const data = await invoke<StorePlugin[]>('fetch_store_index', {
          url: `${BASE_URL}/nodes-index.json`,
        });
        setPlugins(data);
        setError(null);
      } catch (_) {
        // fallback 也失败了，保持 error
      }
    } finally {
      setLoading(false);
    }
  };

  useEffect(() => { fetchIndex(); }, []);

  // ── 安装按钮状态 ──────────────────────────────────────

  const getInstallState = (p: StorePlugin): 'install' | 'installing' | 'installed' => {
    if (installingId === p.id) return 'installing';
    return installedAppIds.has(p.appId) ? 'installed' : 'install';
  };

  // ── 安装 ──────────────────────────────────────────────

  const handleInstall = async (p: StorePlugin) => {
    if (!p.downloadUrl) {
      alert('该插件未提供下载地址');
      return;
    }
    setInstallingId(p.id);
    try {
      await invoke('download_and_install', { url: p.downloadUrl });
      onInstalledChange();
    } catch (err: any) {
      alert(`安装失败: ${err}`);
    } finally {
      setInstallingId(null);
    }
  };

  // ── 查看详情 ──────────────────────────────────────────

  const handleDetail = async (p: StorePlugin) => {
    setDetail(p);
    setDetailReadme(null);
    setDetailFuncList(null);
    setDetailLoading(true);

    const readmeUrl  = `${BASE_URL}/${p.dir}/README.md`;
    const funcUrl    = `${BASE_URL}/${p.dir}/FuncList.json`;

    try {
      // 并行拉 README + FuncList
      const [readme, funcList] = await Promise.all([
        invoke<string>('http_get_text', { url: readmeUrl }).catch(() => '# 暂无说明文档'),
        invoke<string>('http_get_text', { url: funcUrl })
          .then(t => { try { return JSON.parse(t); } catch { return null; } })
          .catch(() => null),
      ]);
      setDetailReadme(readme as string);
      setDetailFuncList(funcList);
    } catch (_) {
      setDetailReadme('# 加载失败');
    } finally {
      setDetailLoading(false);
    }
  };

  // ── 过滤 ──────────────────────────────────────────────

  const filtered = plugins
    .filter(p => typeFilter === 'all' || p.type === typeFilter)
    .filter(p => {
      if (!searchQuery) return true;
      const q = searchQuery.toLowerCase();
      return p.name.toLowerCase().includes(q)
        || p.desc.toLowerCase().includes(q)
        || p.author.toLowerCase().includes(q);
    });

  // ── 安装按钮渲染 ──────────────────────────────────────

  const renderInstallBtn = (p: StorePlugin) => {
    const state = getInstallState(p);
    const btnClass = `${styles.installBtn} ${styles[state]}`;
    const labels: Record<string, string> = {
      install: '⬇️ 安装',
      installing: '⏳ 安装中...',
      installed: '✅ 已安装',
    };
    const disabled = state === 'installing' || state === 'installed';
    return (
      <button
        className={btnClass}
        disabled={disabled}
        onClick={(e) => { e.stopPropagation(); if (!disabled) handleInstall(p); }}
      >
        {labels[state]}
      </button>
    );
  };

  // ── 简单 Markdown 渲染 ────────────────────────────────

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
      if (line.startsWith('```')) return null; // 跳过代码块标记
      if (line.trim() === '') return <br key={i} />;
      return <p key={i}>{line}</p>;
    });
  };

  // ── 渲染 ──────────────────────────────────────────────

  return (
    <>
      {/* 过滤栏 */}
      <div className={styles.filterBar}>
        <div className={styles.typeBtns}>
          {(['all', 'plugin', 'standalone'] as const).map(t => (
            <button
              key={t}
              className="btn btn-sm"
              style={{
                background: typeFilter === t ? '#667eea' : undefined,
                color: typeFilter === t ? '#fff' : undefined,
              }}
              onClick={() => setTypeFilter(t)}
            >
              {t === 'all' ? '全部' : t === 'plugin' ? '🧩 插入式' : '🚀 独立式'}
            </button>
          ))}
        </div>
        <input
          className={styles.searchInput}
          placeholder="🔍 搜索插件..."
          value={searchQuery}
          onChange={e => setSearchQuery(e.target.value)}
        />
        <button className="btn btn-sm" onClick={fetchIndex} style={{ marginLeft: 'auto' }}>
          🔄 刷新
        </button>
      </div>

      {/* 加载 / 错误 / 空 */}
      {loading && <div className="loading">加载商店数据...</div>}
      {!loading && error && (
        <div className={styles.errorBox}>
          ⚠️ 无法连接插件市场: {error}
          <button className="btn btn-sm" onClick={fetchIndex} style={{ marginLeft: 12 }}>重试</button>
        </div>
      )}
      {!loading && !error && filtered.length === 0 && (
        <p style={{ color: '#999', textAlign: 'center', padding: 32 }}>没有匹配的插件</p>
      )}

      {/* 卡片网格 */}
      {!loading && !error && filtered.length > 0 && (
        <div className={styles.cardGrid}>
          {filtered.map(p => (
            <div key={p.id} className={styles.card} onClick={() => handleDetail(p)}>
              <div className={styles.cardLogo}>
                {p.logo ? (
                  <img src={`${BASE_URL}/${p.logo}`} alt={p.name} />
                ) : (
                  <span className={styles.placeholderIcon}>{p.typeIcon}</span>
                )}
              </div>
              <div className={styles.cardBody}>
                <div className={styles.cardName}>{p.name}</div>
                <div className={styles.cardAuthor}>{p.author} · {p.version}</div>
                <div className={styles.cardDesc}>{p.desc}</div>
                <div className={styles.cardMeta}>
                  <span className="status-badge">{p.typeLabel}</span>
                  {p.socketsCount > 0 && <span className="status-badge">📡 {p.socketsCount} 接口</span>}
                  {p.signalsCount > 0 && <span className="status-badge">📢 {p.signalsCount} 信号</span>}
                </div>
              </div>
              <div className={styles.cardFooter}>
                {renderInstallBtn(p)}
              </div>
            </div>
          ))}
        </div>
      )}

      {/* 详情弹窗 */}
      {detail && (
        <div className={styles.modalOverlay} onClick={() => setDetail(null)}>
          <div className={styles.modal} onClick={e => e.stopPropagation()}>
            <button className={styles.modalClose} onClick={() => setDetail(null)}>✕</button>

            {/* 头部 */}
            <div className={styles.modalHeader}>
              {detail.logo ? (
                <img src={`${BASE_URL}/${detail.logo}`} alt={detail.name} className={styles.modalLogo} />
              ) : (
                <span className={styles.modalLogoPlaceholder}>{detail.typeIcon}</span>
              )}
              <div>
                <h2>{detail.name}</h2>
                <p style={{ color: '#6c757d', margin: 0 }}>
                  {detail.author} · {detail.version} · {detail.typeLabel}
                </p>
              </div>
              <div style={{ marginLeft: 'auto' }}>
                {renderInstallBtn(detail)}
              </div>
            </div>

            {/* 内容 */}
            <div className={styles.modalContent}>
              {detailLoading ? (
                <div className="loading">加载详情...</div>
              ) : (
                <>
                  {/* FuncList 接口 */}
                  {detailFuncList?.openSocket && Object.keys(detailFuncList.openSocket).length > 0 && (
                    <div className={styles.modalSection}>
                      <h3>⚡ 功能接口</h3>
                      <div className={styles.funcTable}>
                        <div className={styles.funcHeader}>
                          <span>接口名</span><span>ID</span><span>描述</span><span>参数</span>
                        </div>
                        {Object.entries(detailFuncList.openSocket).map(([key, val]: [string, any]) => (
                          <div key={key} className={styles.funcRow}>
                            <span><strong>{key}</strong></span>
                            <span><code>{val.openSocketID}</code></span>
                            <span>{val.description}</span>
                            <span>{val.args ? Object.keys(val.args).join(', ') : '-'}</span>
                          </div>
                        ))}
                      </div>
                    </div>
                  )}

                  {/* Signal */}
                  {detailFuncList?.signal && Object.keys(detailFuncList.signal).length > 0 && (
                    <div className={styles.modalSection}>
                      <h3>📢 信号</h3>
                      {Object.entries(detailFuncList.signal).map(([key, val]: [string, any]) => (
                        <div key={key} style={{ marginBottom: 8 }}>
                          <strong>{key}</strong> ({val.signalID}): {val.description}
                        </div>
                      ))}
                    </div>
                  )}

                  {/* README */}
                  {detailReadme && detailReadme !== '# 暂无说明文档' && (
                    <div className={styles.modalSection}>
                      <h3>📖 说明</h3>
                      <div className={styles.readme}>{renderMarkdown(detailReadme)}</div>
                    </div>
                  )}

                  {/* 无内容 */}
                  {!detailReadme && !detailFuncList && (
                    <p style={{ color: '#999', textAlign: 'center', padding: 32 }}>暂无详情信息</p>
                  )}
                </>
              )}
            </div>
          </div>
        </div>
      )}
    </>
  );
}
