import { useState, useEffect } from 'react';
import { invoke } from '@tauri-apps/api/core';
import { openUrl } from '@tauri-apps/plugin-opener';
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
  const [typeFilter, setTypeFilter] = useState<'all' | 'plugin' | 'standalone'>('plugin');
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

  /** 去掉 v 前缀，比较三段数字。只有商店版本严格更高时才需更新 */
  const isNewer = (storeVer: string, localVer: string): boolean => {
    const toNums = (v: string) => v.replace(/^v/i, '').split('.').map(s => parseInt(s, 10) || 0);
    const s = toNums(storeVer);
    const l = toNums(localVer);
    for (let i = 0; i < Math.max(s.length, l.length); i++) {
      if ((s[i] || 0) > (l[i] || 0)) return true;
      if ((s[i] || 0) < (l[i] || 0)) return false;
    }
    return false; // 相等
  };

  const getInstallState = (p: StorePlugin): 'install' | 'installing' | 'installed' | 'update' => {
    if (installingId === p.id) return 'installing';
    const localVer = installedAppIds.get(p.appId);
    if (!localVer) return 'install';
    if (p.version && isNewer(p.version, localVer)) return 'update';
    return 'installed';
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

  // ── 监听安装完成（来自 PreviewBar 的安装操作） ─────

  useEffect(() => {
    const handler = () => onInstalledChange();
    window.addEventListener('plugin-installed', handler);
    return () => window.removeEventListener('plugin-installed', handler);
  }, [onInstalledChange]);

  // ── 查看详情 → 右侧 PreviewBar ───────────────────────

  const handleRowClick = (p: StorePlugin) => {
    window.dispatchEvent(new CustomEvent('update-preview', {
      detail: {
        type: 'store',
        id: p.id,
        details: { plugin: p, installed: installedAppIds.has(p.appId) },
      },
    }));
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
    // 独立式节点：打开浏览器到下载页
    if (p.type === 'standalone') {
      const isRegistered = installedAppIds.has(p.appId);
      return (
        <button
          className={`${styles.installBtn} ${isRegistered ? styles.installed : styles.install}`}
          disabled={isRegistered}
          onClick={(e) => {
            e.stopPropagation();
            if (p.downloadUrl) openUrl(p.downloadUrl);
          }}
        >
          {isRegistered ? '✅ 已注册' : '🌐 下载'}
        </button>
      );
    }

    // 插入式节点：安装/已安装/更新
    const state = getInstallState(p);
    const btnClass = `${styles.installBtn} ${styles[state]}`;
    const labels: Record<string, string> = {
      install: '⬇️ 安装',
      installing: '⏳ 安装中...',
      installed: '✅ 已安装',
      update: '⬆️ 更新',
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

      {/* 横行列表 */}
      {!loading && !error && filtered.length > 0 && (
        <div className={styles.rowList}>
          {filtered.map(p => (
            <div key={p.id} className={styles.row} onClick={() => handleRowClick(p)}>
              {/* Logo */}
              <div className={styles.rowLogo}>
                {p.logo ? (
                  <img src={`${BASE_URL}/${p.logo}`} alt={p.name} />
                ) : (
                  <span className={styles.placeholderIcon}>{p.typeIcon}</span>
                )}
              </div>
              {/* 信息 */}
              <div className={styles.rowInfo}>
                <span className={styles.rowName}>{p.name}</span>
                <span className={styles.rowAuthor}>{p.author} · {p.version}</span>
                {p.desc && <span className={styles.rowDesc}>{p.desc}</span>}
              </div>
              {/* 标签 */}
              <div className={styles.rowMeta}>
                {p.socketsCount > 0 && <span className="status-badge">📡 {p.socketsCount}</span>}
                {p.signalsCount > 0 && <span className="status-badge">📢 {p.signalsCount}</span>}
              </div>
              {/* 安装 */}
              {renderInstallBtn(p)}
            </div>
          ))}
        </div>
      )}

    </>
  );
}
