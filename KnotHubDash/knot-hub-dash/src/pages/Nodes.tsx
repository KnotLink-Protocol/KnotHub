import { useState, useEffect } from 'react';
import { invoke } from '@tauri-apps/api/core';
import styles from './Nodes.module.css';
import deleteIcon from '../assets/delete.svg';
import startIcon from '../assets/start.svg';
import stopIcon from '../assets/stop.svg';
import settingIcon from '../assets/setting.svg';
import homeIcon from '../assets/home.svg';

interface NodeSummary {
  id: string;
  app_id: string;
  role: string;
  status: string;
  hot_role: string;
  author: string;
  version: string;
  node_type: string;
  name?: string;
  auto_start?: string;
  description?: string;
}

type TabKey = 'plugin' | 'standalone';

export default function Nodes() {
  const [tab, setTab] = useState<TabKey>('plugin');
  const [plugins, setPlugins] = useState<NodeSummary[]>([]);
  const [standalones, setStandalones] = useState<NodeSummary[]>([]);
  const [loading, setLoading] = useState(true);

  const fetchAll = async () => {
    setLoading(true);
    try {
      const [p, s] = await Promise.all([
        invoke<NodeSummary[]>('get_plugin_list'),
        invoke<NodeSummary[]>('get_standalone_list'),
      ]);
      setPlugins(p);
      setStandalones(s);
    } catch (err) {
      console.error('获取节点列表失败:', err);
    } finally {
      setLoading(false);
    }
  };

  useEffect(() => { fetchAll(); }, []);

  const nodes = tab === 'plugin' ? plugins : standalones;

  const handleItemClick = (nodeId: string) => {
    window.dispatchEvent(new CustomEvent('update-preview', {
      detail: { type: 'node', id: nodeId, nodeType: tab },
    }));
  };

  const handleAction = async (action: string, nodeId: string, e: React.MouseEvent) => {
    e.stopPropagation();
    // 独立式节点无操作按钮，仅插件走此路径
    const setter = setPlugins;

    try {
      switch (action) {
        case 'start':
          await invoke('start_plugin', { nodeId });
          setter(prev => prev.map(n => n.id === nodeId ? { ...n, status: '运行中' } : n));
          break;
        case 'stop':
          await invoke('stop_plugin', { nodeId });
          setter(prev => prev.map(n => n.id === nodeId ? { ...n, status: '停止' } : n));
          break;
        case 'delete':
          if (confirm(`确定要删除节点 ${nodeId} 吗？`)) {
            await invoke('delete_node', { pluginName: nodeId });
            setter(prev => prev.filter(n => n.id !== nodeId));
            window.dispatchEvent(new CustomEvent('update-preview', { detail: null }));
            await fetchAll();
          }
          break;
        case 'settings':
          alert('设置功能开发中');
          break;
        case 'home':
          await invoke('open_node_home', { pluginName: nodeId });
          break;
        default:
          alert(`未知操作: ${action}`);
      }
    } catch (err) {
      await fetchAll();
      alert(`操作失败: ${err}`);
    }
  };

  const handleToggleAutostart = async (nodeId: string, current: string, e: React.MouseEvent) => {
    e.stopPropagation();
    const enable = current !== 'true';
    try {
      if (tab === 'plugin') {
        await invoke('set_plugin_autostart', { nodeId, autoStart: enable });
      } else {
        // 独立式暂不支持远端修改 autostart
        return;
      }
      const setter = tab === 'plugin' ? setPlugins : setStandalones;
      setter(prev => prev.map(n =>
        n.id === nodeId ? { ...n, auto_start: enable ? 'true' : 'false' } : n
      ));
    } catch (err) {
      alert(`设置自启失败: ${err}`);
    }
  };

  const handleRefresh = async () => {
    setLoading(true);
    try {
      if (tab === 'plugin') {
        setPlugins(await invoke<NodeSummary[]>('refresh_plugins'));
      } else {
        setStandalones(await invoke<NodeSummary[]>('refresh_standalone'));
      }
    } catch (err) {
      console.error('刷新失败:', err);
    } finally {
      setLoading(false);
    }
  };

  const getStatusClass = (status: string) => {
    if (status === '已注册') return 'status-badge';
    return status === '运行中' ? 'status-badge' : 'status-badge warning';
  };

  return (
    <>
      <div style={{ marginBottom: 16 }}>
        <h1 style={{ fontSize: 22, fontWeight: 500 }}>节点列表</h1>
        <p style={{ color: '#6c757d' }}>
          {tab === 'plugin' ? '插入式节点 — 本地插件管理' : '独立式节点 — 注册表发现的外部节点'}
        </p>
      </div>

      {/* Tab 切换 */}
      <div style={{ display: 'flex', gap: 8, marginBottom: 16 }}>
        {(['plugin', 'standalone'] as TabKey[]).map(k => (
          <button
            key={k}
            className="btn"
            style={{
              background: tab === k ? '#667eea' : undefined,
              color: tab === k ? '#fff' : undefined,
            }}
            onClick={() => setTab(k)}
          >
            {k === 'plugin' ? '🔌 插入式节点' : '📡 独立式节点'}
            <span style={{ marginLeft: 8, opacity: 0.7, fontSize: 12 }}>
              {k === 'plugin' ? plugins.length : standalones.length}
            </span>
          </button>
        ))}
        <button className="btn btn-sm" onClick={handleRefresh} style={{ marginLeft: 'auto' }}>
          🔄 刷新
        </button>
      </div>

      {loading ? (
        <div className="loading">加载中...</div>
      ) : (
        <div className="section">
          <div className={styles.nodesList}>
            {nodes.length === 0 && (
              <p style={{ color: '#999', textAlign: 'center', padding: 32 }}>
                暂无{tab === 'plugin' ? '插入式' : '独立式'}节点
              </p>
            )}
            {nodes.map(node => (
              <div
                key={node.id}
                className={styles.nodeCard}
                onClick={() => handleItemClick(node.id)}
              >
                <div className={styles.top}>
                  <span className={styles.appId}>{node.app_id}</span>
                  <div className={styles.topRight}>
                    <span className={getStatusClass(node.status)}>{node.status}</span>
                    {node.node_type !== 'standalone' && (
                      <button
                        className={`btn btn-sm ${styles.iconBtn} ${styles.deleteIcon} btn-danger ${styles.noBorder}`}
                        aria-label="删除"
                        onClick={(e) => handleAction('delete', node.id, e)}
                      >
                        <img src={deleteIcon} alt="删除" className={styles.deleteSvg} />
                      </button>
                    )}
                  </div>
                </div>

                <div className={styles.middle}>
                  <div className={styles.infoLeft}>
                    <span className={styles.icon}>
                      {tab === 'standalone' ? '📡' : '🖥️'}
                    </span>
                    <span className={styles.name}>
                      {node.name || node.id}
                    </span>
                  </div>
                  <div className={styles.actions}>
                    {tab !== 'standalone' && (
                      <button
                        className={`btn btn-sm ${styles.iconBtn} ${styles.noBorder}`}
                        aria-label={node.status === '运行中' ? '停止' : '启动'}
                        onClick={(e) => handleAction(
                          node.status === '运行中' ? 'stop' : 'start', node.id, e)}
                      >
                        <img
                          src={node.status === '运行中' ? stopIcon : startIcon}
                          alt={node.status === '运行中' ? '停止' : '启动'}
                          className={styles.actionIcon}
                        />
                      </button>
                    )}
                    {tab !== 'standalone' && (
                      <button
                        className={`btn btn-sm ${styles.iconBtn} ${styles.noBorder}`}
                        aria-label="设置"
                        onClick={(e) => handleAction('settings', node.id, e)}
                      >
                        <img src={settingIcon} alt="设置" className={styles.actionIcon} />
                      </button>
                    )}
                    {tab !== 'standalone' && (
                      <button
                        className={`btn btn-sm ${styles.iconBtn} ${styles.noBorder}`}
                        aria-label="主页"
                        onClick={(e) => handleAction('home', node.id, e)}
                      >
                        <img src={homeIcon} alt="主页" className={styles.actionIcon} />
                      </button>
                    )}
                  </div>
                </div>

                <div className={styles.bottom}>
                  <span className={styles.author}>{node.author}</span>
                  <span className={styles.version}>{node.version}</span>
                  {tab !== 'standalone' && (
                    <button
                      className="btn btn-sm"
                      style={{
                        fontSize: 11,
                        marginLeft: 8,
                        padding: '2px 8px',
                        background: node.auto_start === 'true' ? '#667eea' : '#ddd',
                        color: node.auto_start === 'true' ? '#fff' : '#666',
                        border: 'none',
                        borderRadius: 10,
                        cursor: 'pointer',
                      }}
                      onClick={(e) => handleToggleAutostart(node.id, node.auto_start || 'false', e)}
                      title={node.auto_start === 'true' ? '已开启自启，点击关闭' : '已关闭自启，点击开启'}
                    >
                      {node.auto_start === 'true' ? '自启 ✓' : '自启 ✗'}
                    </button>
                  )}
                </div>
              </div>
            ))}
          </div>
        </div>
      )}
    </>
  );
}
