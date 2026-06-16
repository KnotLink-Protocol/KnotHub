import { useState, useEffect } from 'react';
import { invoke } from '@tauri-apps/api/core';
import styles from './Nodes.module.css';
import deleteIcon from '../assets/delete.svg';
import startIcon from '../assets/start.svg';
import stopIcon from '../assets/stop.svg';
import settingIcon from '../assets/setting.svg';
import homeIcon from '../assets/home.svg';

// 定义后端返回的数据结构
interface NodeSummary {
  id: string;
  app_id: string;
  role: string;
  status: string;
  hot_role: string;
  author: string;
  version: string;
}

export default function Nodes() {
  const [nodes, setNodes] = useState<NodeSummary[]>([]);
  const [loading, setLoading] = useState(true);

  // 获取节点列表
  const fetchNodes = async () => {
    try {
      const data = await invoke<NodeSummary[]>('get_nodes_list');
      setNodes(data);
    } catch (err) {
      console.error('获取节点列表失败:', err);
    } finally {
      setLoading(false);
    }
  };

  useEffect(() => {
    fetchNodes();
  }, []);

  const handleItemClick = (nodeId: string) => {
    window.dispatchEvent(new CustomEvent('update-preview', { detail: { type: 'node', id: nodeId } }));
  };

  const handleAction = (action: string, nodeId: string, e: React.MouseEvent) => {
    e.stopPropagation();
    // TODO: 对接真实 Tauri 命令
    alert(`[演示] 对节点 ${nodeId} 执行操作: ${action}`);
  };

  // 根据状态生成 CSS 类名
  const getStatusClass = (status: string) => {
    if (status === '运行中') return 'status-badge';
    if (status === '停止') return 'status-badge warning';
    return 'status-badge';
  };

  if (loading) return <div className="loading">加载节点列表中...</div>;

  return (
    <>
      <div style={{ marginBottom: 16 }}>
        <h1 style={{ fontSize: 22, fontWeight: 500 }}>节点列表</h1>
        <p style={{ color: '#6c757d' }}>服务节点管理 · 插入式结构</p>
      </div>
      <div className="section">
        <div className="section-header">
          <div className="section-title">节点管理</div>
          <button className="btn btn-sm" onClick={() => alert('安装插件（演示）')}>
            安装插件
          </button>
        </div>
        <div className={styles.nodesList}>
          {nodes.map((node) => (
            <div
              key={node.id}
              className={styles.nodeCard}
              onClick={() => handleItemClick(node.id)}
            >
              {/* 上区域 */}
              <div className={styles.top}>
                <span className={styles.appId}>{node.app_id}</span>
                <div className={styles.topRight}>
                  <span className={getStatusClass(node.status)}>{node.status}</span>
                  <button
                    className={`btn btn-sm ${styles.iconBtn} ${styles.deleteIcon} btn-danger ${styles.noBorder}`}
                    aria-label="删除"
                    onClick={(e) => handleAction('delete', node.id, e)}
                  >
                    <img src={deleteIcon} alt="删除" className={styles.deleteSvg} />
                  </button>
                </div>
              </div>

              {/* 中区域 */}
              <div className={styles.middle}>
                <div className={styles.infoLeft}>
                  <span className={styles.icon}>🖥️</span>
                  <span className={styles.name}>{node.id}</span>
                </div>
                <div className={styles.actions}>
                  <button
                    className={`btn btn-sm ${styles.iconBtn} ${styles.noBorder}`}
                    aria-label={node.status === '运行中' ? '停止' : '启动'}
                    onClick={(e) => handleAction(node.status === '运行中' ? 'stop' : 'start', node.id, e)}
                  >
                    <img
                      src={node.status === '运行中' ? stopIcon : startIcon}
                      alt={node.status === '运行中' ? '停止' : '启动'}
                      className={styles.actionIcon}
                    />
                  </button>
                  <button
                    className={`btn btn-sm ${styles.iconBtn} ${styles.noBorder}`}
                    aria-label="设置"
                    onClick={(e) => handleAction('settings', node.id, e)}
                  >
                    <img src={settingIcon} alt="设置" className={styles.actionIcon} />
                  </button>
                  <button
                    className={`btn btn-sm ${styles.iconBtn} ${styles.noBorder}`}
                    aria-label="主页"
                    onClick={(e) => handleAction('home', node.id, e)}
                  >
                    <img src={homeIcon} alt="主页" className={styles.actionIcon} />
                  </button>
                </div>
              </div>

              {/* 下区域 */}
              <div className={styles.bottom}>
                <span className={styles.author}>{node.author}</span>
                <span className={styles.version}>{node.version}</span>
              </div>
            </div>
          ))}
        </div>
      </div>
    </>
  );
}