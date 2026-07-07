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
  const [collapsed, setCollapsed] = useState<Record<string, boolean>>({});

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

  useEffect(() => { loadTree(); }, [loadTree]);

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
        detail: { type: 'recipe', id: node.id, details: { id: node.id, name: node.name, status } },
      }));
    }
  };

  const toggleFolder = (id: string) => {
    setCollapsed(prev => ({ ...prev, [id]: !prev[id] }));
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

  const getStatus = (node: TreeNode) => statusMap[node.id] || node.status || 'stopped';

  const renderTree = (node: TreeNode) => {
    const isFolder = node.type === 'folder';
    const folded = collapsed[node.id] === true;

    if (isFolder) {
      return (
        <div key={node.id}>
          <div className={styles.folderRow} onClick={() => toggleFolder(node.id)}>
            <span style={{ marginRight: 6 }}>{folded ? '▶' : '▼'}</span>
            <span>{folded ? '📁' : '📂'} {node.name}</span>
            <span style={{ fontSize: 11, color: '#999', marginLeft: 8 }}>
              {(node.children || []).length} 项
            </span>
            <button
              className="btn btn-sm"
              style={{ marginLeft: 'auto', fontSize: 11, padding: '1px 6px' }}
              onClick={(e) => { e.stopPropagation(); handleNew(node.id); }}
            >
              +
            </button>
          </div>
          {!folded && (
            <div className={styles.treeLevel}>
              {(node.children || []).map(child => renderTree(child))}
            </div>
          )}
        </div>
      );
    }

    // recipe — compact
    const status = getStatus(node);
    return (
      <div key={node.id} className={styles.recipeRow} onClick={() => handleClick(node)}>
        <span style={{ marginRight: 6, fontSize: 10, color: '#999' }}>├</span>
        <span style={{ marginRight: 4 }}>🐍</span>
        <span>{node.name}</span>
        <span
          className={status === 'running' ? 'status-badge' : 'status-badge warning'}
          style={{ fontSize: 10, padding: '0px 5px', lineHeight: '16px', marginLeft: 8 }}
        >
          {status === 'running' ? '运行' : '停'}
        </span>
        <div style={{ marginLeft: 'auto', display: 'flex', gap: 3 }}>
          {status === 'running' ? (
            <button className="btn btn-sm btn-danger" style={{ fontSize: 10, padding: '0px 5px' }}
              onClick={(e) => handleStop(node.id, e)}>⏹</button>
          ) : (
            <button className="btn btn-sm" style={{ fontSize: 10, padding: '0px 5px' }}
              onClick={(e) => handleRun(node.id, e)}>▶</button>
          )}
          <button className="btn btn-sm btn-danger" style={{ fontSize: 10, padding: '0px 5px' }}
            onClick={(e) => handleDelete(node.id, e)}>🗑</button>
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
