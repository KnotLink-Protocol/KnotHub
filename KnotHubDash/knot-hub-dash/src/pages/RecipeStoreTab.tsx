import { useState, useEffect } from 'react';
import { Channel, invoke } from '@tauri-apps/api/core';
import { subscribe, getDlState, updateProgress } from '../lib/downloadState';
import styles from './RecipeStoreTab.module.css';

// ── 数据结构（与 recipes-data.js 对齐）──────────────────

interface RecipeLink {
  app_id: string;
  app_name: string;
  min_version: string;
  role: string;
}

interface StoreRecipe {
  id: string;
  name: string;
  version: string;
  author: string;
  description: string;
  icon: string;
  tags: string[];
  links: RecipeLink[];
  created: string;
  updated: string;
  path: string;
}

const BASE_URL = 'https://knotlink.cn';

interface Props {
  installedAppIds: Map<string, string>;
  onInstalledChange: () => void;
}

export default function RecipeStoreTab({ installedAppIds, onInstalledChange }: Props) {
  const [recipes, setRecipes] = useState<StoreRecipe[]>([]);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState<string | null>(null);
  const [installingId, setInstallingId] = useState<string | null>(null);
  const [tagFilter, setTagFilter] = useState<string | null>(null);
  const [searchQuery, setSearchQuery] = useState('');
  const [importedNames, setImportedNames] = useState<Set<string>>(new Set());
  const [dl, setDl] = useState(getDlState());

  // 订阅下载进度
  useEffect(() => subscribe(() => setDl(getDlState())), []);

  // ── 提取已导入配方名 ──────────────────────────────────

  const extractImportedNames = (node: any): Set<string> => {
    const names = new Set<string>();
    const walk = (n: any) => {
      if (n.type === 'recipe') names.add(n.name);
      n.children?.forEach(walk);
    };
    walk(node);
    return names;
  };

  // ── 加载数据 ──────────────────────────────────────────

  const loadData = async () => {
    setLoading(true);
    setError(null);
    try {
      // 拉 recipes-data.js
      const js = await invoke<string>('http_get_text', {
        url: `${BASE_URL}/recipes-data.js`,
      });
      const match = js.match(/window\.__KNOTLINK_RECIPES__\s*=\s*(\[[\s\S]*?\]);/);
      if (!match) throw new Error('无法解析 recipes-data.js');
      const data: StoreRecipe[] = JSON.parse(match[1]);
      setRecipes(data);

      // 拉已导入列表
      const tree = await invoke<any>('get_recipe_tree');
      setImportedNames(extractImportedNames(tree));
    } catch (err: any) {
      setError(typeof err === 'string' ? err : (err?.message || String(err)));
    } finally {
      setLoading(false);
    }
  };

  useEffect(() => { loadData(); }, []);

  // ── 安装 ──────────────────────────────────────────────

  const handleInstall = async (r: StoreRecipe) => {
    const url = `${BASE_URL}/recipes-market/${r.path}`;
    setInstallingId(r.id);
    try {
      const ch = new Channel<{ downloaded: number; total: number | null }>((msg) => {
        updateProgress(msg.downloaded, msg.total);
      });
      await invoke('download_and_import_recipe', { url, onProgress: ch });
      setImportedNames(prev => new Set([...prev, r.path.split('/').pop() || r.path]));
      window.dispatchEvent(new Event('recipe-installed'));
      onInstalledChange();
    } catch (err: any) {
      alert(`安装失败: ${err}`);
    } finally {
      setInstallingId(null);
    }
  };

  // ── 预览 ──────────────────────────────────────────────

  const handleRowClick = (r: StoreRecipe) => {
    window.dispatchEvent(new CustomEvent('update-preview', {
      detail: {
        type: 'recipe-store',
        id: r.id,
        details: { recipe: r, installed: isImported(r) },
      },
    }));
  };

  // ── 辅助 ──────────────────────────────────────────────

  const isImported = (r: StoreRecipe): boolean => {
    const name = r.path.split('/').pop() || '';
    return importedNames.has(name);
  };

  // ── 收集所有标签 ──────────────────────────────────────

  const allTags = [...new Set(recipes.flatMap(r => r.tags || []))].sort();

  // ── 过滤 ──────────────────────────────────────────────

  const filtered = recipes.filter(r => {
    if (tagFilter && !(r.tags || []).includes(tagFilter)) return false;
    if (searchQuery) {
      const q = searchQuery.toLowerCase();
      return r.name.toLowerCase().includes(q)
        || r.description.toLowerCase().includes(q)
        || r.author.toLowerCase().includes(q)
        || r.links.some(l => l.app_name.toLowerCase().includes(q));
    }
    return true;
  });

  // ── links 中已安装的节点 ──────────────────────────────

  const linkInstalled = (link: RecipeLink) => installedAppIds.has(link.app_id);

  // ── 渲染 ──────────────────────────────────────────────

  return (
    <>
      {/* 标题 */}
      <div style={{ marginBottom: 16 }}>
        <h1 style={{ fontSize: 22, fontWeight: 500 }}>配方市场</h1>
        <p style={{ color: '#6c757d' }}>
          编排脚本，串联多个 KnotLink 节点实现自动化流程
        </p>
      </div>

      {/* 过滤栏 */}
      <div className={styles.filterBar}>
        <div className={styles.tagBtns}>
          <button
            className="btn btn-sm"
            style={{ background: !tagFilter ? '#667eea' : undefined, color: !tagFilter ? '#fff' : undefined }}
            onClick={() => setTagFilter(null)}
          >
            全部
          </button>
          {allTags.map(tag => (
            <button
              key={tag}
              className="btn btn-sm"
              style={{ background: tagFilter === tag ? '#667eea' : undefined, color: tagFilter === tag ? '#fff' : undefined }}
              onClick={() => setTagFilter(tagFilter === tag ? null : tag)}
            >
              {tag}
            </button>
          ))}
        </div>
        <input
          className={styles.searchInput}
          placeholder="🔍 搜索配方..."
          value={searchQuery}
          onChange={e => setSearchQuery(e.target.value)}
        />
        <button className="btn btn-sm" onClick={loadData} style={{ marginLeft: 'auto' }}>
          🔄 刷新
        </button>
      </div>

      {/* 加载 / 错误 / 空 */}
      {loading && <div className="loading">加载配方数据...</div>}
      {!loading && error && (
        <div className={styles.errorBox}>
          ⚠️ 无法连接配方市场: {error}
          <button className="btn btn-sm" onClick={loadData} style={{ marginLeft: 12 }}>重试</button>
        </div>
      )}
      {!loading && !error && filtered.length === 0 && (
        <p style={{ color: '#999', textAlign: 'center', padding: 32 }}>没有匹配的配方</p>
      )}

      {/* 横行列表 */}
      {!loading && !error && filtered.length > 0 && (
        <div className={styles.rowList}>
          {filtered.map(r => (
            <div key={r.id} className={styles.row} onClick={() => handleRowClick(r)}>
              {/* 图标 */}
              <div className={styles.rowLogo}>
                <span className={styles.placeholderIcon}>{r.icon}</span>
              </div>
              {/* 信息 */}
              <div className={styles.rowInfo}>
                <span className={styles.rowName}>{r.name}</span>
                <span className={styles.rowAuthor}>{r.author} · {r.version}</span>
                {r.description && <span className={styles.rowDesc}>{r.description}</span>}
              </div>
              {/* 串联节点 */}
              <div className={styles.rowLinks}>
                {r.links.map(link => (
                  <span
                    key={link.app_id}
                    className={`${styles.linkBadge} ${linkInstalled(link) ? styles.linkOk : styles.linkMissing}`}
                    title={`${link.app_name}: ${link.role}`}
                  >
                    {linkInstalled(link) ? '✅' : '⚠️'} {link.app_name}
                  </span>
                ))}
              </div>
              {/* 安装 */}
              <div className={styles.rowInstall}>
                {isImported(r) ? (
                  <button className={`${styles.installBtn} ${styles.installed}`} disabled>
                    ✅ 已导入
                  </button>
                ) : installingId === r.id ? (
                  <button className={`${styles.installBtn} ${styles.installing}`} disabled>
                    {dl.downloading && dl.percent > 0 ? (
                      <span style={{ display: 'flex', alignItems: 'center', gap: 4 }}>
                        ⏳ {dl.percent}%
                        <progress value={dl.percent} max={100}
                          style={{ width: 48, height: 8, accentColor: '#667eea' }} />
                      </span>
                    ) : '⏳ 安装中...'}
                  </button>
                ) : (
                  <button
                    className={`${styles.installBtn} ${styles.install}`}
                    onClick={e => { e.stopPropagation(); handleInstall(r); }}
                  >
                    ⬇️ 安装
                  </button>
                )}
              </div>
            </div>
          ))}
        </div>
      )}
    </>
  );
}
