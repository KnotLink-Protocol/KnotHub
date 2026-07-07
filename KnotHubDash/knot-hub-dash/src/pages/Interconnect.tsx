import { useState, useEffect, useCallback } from 'react';
import { invoke } from '@tauri-apps/api/core';
import styles from './Interconnect.module.css';

interface TreeNode {
  id: string;
  name: string;
  type: string;
  children?: TreeNode[];
  status?: string;
}

export default function Interconnect() {
  const [tree, setTree] = useState<TreeNode | null>(null);
  const [loading, setLoading] = useState(true);
  const [statusMap, setStatusMap] = useState<Record<string, string>>({});

  const loadTree = useCallback(async () => {
    try {
      setLoading(true);
      const data = await invoke<TreeNode>('get_recipe_tree');
      setTree(data);
    } catch (err) {
      console.error('加载配方树失败:', err);
    } finally {
      setLoading(false);
    }
  }, []);

  useEffect(() => {
    loadTree();
  }, [loadTree]);

  const handleRun = async (filePath: string, e: React.MouseEvent) => {
    e.stopPropagation();
    try {
      await invoke('recipe_run', { filePath });
      setStatusMap(prev => ({ ...prev, [filePath]: 'running' }));
    } catch (err) {
      alert(`运行失败: ${err}`);
    }
  };

  const handleStop = async (filePath: string, e: React.MouseEvent) => {
    e.stopPropagation();
    try {
      await invoke('recipe_stop', { filePath });
      setStatusMap(prev => ({ ...prev, [filePath]: 'stopped' }));
    } catch (err) {
      alert(`停止失败: ${err}`);
    }
  };

  const handleDelete = async (filePath: string, e: React.MouseEvent) => {
    e.stopPropagation();
    if (!confirm('确定删除？')) return;
    try {
      await invoke('recipe_delete', { filePath });
      loadTree();
    } catch (err) {
      alert(`删除失败: ${err}`);
    }
  };

  const handleClick = (node: TreeNode) => {
    if (node.type === 'recipe') {
      const status = statusMap[node.id] || node.status || 'stopped';
      window.dispatchEvent(new CustomEvent('update-preview', {
        detail: {
          type: 'recipe',
          id: node.id,
          details: { id: node.id, name: node.name, status },
        },
      }));
    }
  };

  const handleNew = async (parentPath: string) => {
    const name = prompt('配方文件名（含 .py）：');
    if (!name) return;
    const filePath = parentPath + '/' + name;
    try {
      await invoke('recipe_save', { filePath, content: '# 新配方\nprint("hello")' });
      loadTree();
    } catch (err) {
      alert(`创建失败: ${err}`);
    }
  };

  const getStatus = (node: TreeNode) => {
    return statusMap[node.id] || node.status || 'stopped';
  };

  const renderTree = (node: TreeNode, depth: number = 0) => {
    const status = getStatus(node);

    if (node.type === 'folder') {
      return (
        <div key={node.id} style={{ marginLeft: depth * 20 }}>
          <div className={styles.treeNode}>
            <span>📁 {node.name}</span>
            <button className="btn btn-sm" onClick={() => handleNew(node.id)}>+新建</button>
          </div>
          {(node.children || []).map(child => renderTree(child, depth + 1))}
        </div>
      );
    }

    return (
      <div key={node.id} style={{ marginLeft: depth * 20 }}>
        <div
          className={styles.treeNode}
          onClick={() => handleClick(node)}
          style={{ cursor: 'pointer' }}
        >
          <span className={styles.recipeIcon}>🐍</span>
          <span className={styles.nodeName}>{node.name}</span>
          <span className={status === 'running' ? 'status-badge' : 'status-badge warning'}>
            {status}
          </span>
          {status === 'running' ? (
            <button className="btn btn-sm btn-danger" onClick={(e) => handleStop(node.id, e)}>
              ⏹ 停止
            </button>
          ) : (
            <button className="btn btn-sm" onClick={(e) => handleRun(node.id, e)}>
              ▶ 运行
            </button>
          )}
          <button className="btn btn-sm btn-danger" onClick={(e) => handleDelete(node.id, e)}>
            🗑
          </button>
        </div>
      </div>
    );
  };

  if (loading) return <div className="loading">加载配方树...</div>;

  return (
    <>
      <div style={{ marginBottom: 16 }}>
        <h1 style={{ fontSize: 22, fontWeight: 500 }}>互联配方</h1>
        <p style={{ color: '#6c757d' }}>配方文件管理 · 运行 Python 配方</p>
      </div>
      <div className="section">
        <div className="section-header">
          <div className="section-title">配方树</div>
          <div style={{ display: 'flex', gap: 8 }}>
            <button className="btn btn-sm" onClick={loadTree}>🔄 刷新</button>
            <button className="btn btn-sm" onClick={() => handleNew(tree?.id || 'Recipes')}>
              新建配方
            </button>
          </div>
        </div>
        {tree && renderTree(tree)}
      </div>
    </>
  );
}
