import { useState, useEffect, useCallback, useRef } from 'react';
import { invoke } from '@tauri-apps/api/core';
import { getCurrentWindow } from '@tauri-apps/api/window';
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

  // ── 拖拽状态 ──────────────────────────────────────────────
  const [dragOver, setDragOver] = useState(false);
  const [dragTarget, setDragTarget] = useState<string | null>(null);
  const [importMsg, setImportMsg] = useState<string | null>(null);
  const dragTargetRef = useRef<string | null>(null);

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

  // ── 拖拽事件 ────────────────────────────────────────────
  const handleDrop = useCallback(async (paths: string[]) => {
    setDragOver(false);
    setDragTarget(null);
    const file = paths.find(p => p.endsWith('.py') || p.endsWith('.kln'));
    if (!file) {
      setImportMsg('请拖入 .py 或 .kln 文件');
      return;
    }
    const target = dragTargetRef.current || tree?.id || 'Recipes';
    setImportMsg('导入中...');
    try {
      await invoke('recipe_import', {
        sourcePath: file,
        targetDir: target,
        overwrite: false,
      });
      setImportMsg(`已导入: ${file.split('\\').pop()}`);
      loadTree();
    } catch (err: any) {
      if (String(err).includes('file exists')) {
        const ok = confirm('文件已存在，是否覆盖？');
        if (ok) {
          try {
            await invoke('recipe_import', {
              sourcePath: file,
              targetDir: target,
              overwrite: true,
            });
            setImportMsg(`已覆盖: ${file.split('\\').pop()}`);
            loadTree();
          } catch (e) {
            setImportMsg(`导入失败: ${e}`);
          }
        } else {
          setImportMsg(null);
        }
      } else {
        setImportMsg(`导入失败: ${err}`);
      }
    }
  }, [tree]);

  useEffect(() => {
    const unlisten = getCurrentWindow().onDragDropEvent((event) => {
      if (event.payload.type === 'over') {
        setDragOver(true);
      } else if (event.payload.type === 'leave') {
        setDragOver(false);
        setDragTarget(null);
        dragTargetRef.current = null;
      } else if (event.payload.type === 'drop') {
        handleDrop(event.payload.paths);
      }
    });
    return () => {
      unlisten.then(fn => fn());
    };
  }, [handleDrop]);

  // ── 运行 / 停止 ─────────────────────────────────────────
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

  // ── 新建（文件或文件夹）──────────────────────────────────
  const handleNew = async (parentPath: string) => {
    const input = prompt('新建（输入 "名称.py" 建文件，输入 "名称/" 建文件夹）：');
    if (!input) return;

    const fullPath = parentPath + '/' + input;

    if (input.endsWith('/')) {
      // 创建文件夹
      const dirPath = fullPath.slice(0, -1); // 去掉末尾 /
      try {
        await invoke('recipe_create_folder', { path: dirPath });
        loadTree();
      } catch (err) {
        alert(`创建文件夹失败: ${err}`);
      }
    } else {
      // 创建文件
      const name = input.endsWith('.py') || input.endsWith('.kln') ? input : input + '.py';
      const filePath = parentPath + '/' + name;
      try {
        await invoke('recipe_save', { filePath, content: '# 新配方\nprint("hello")' });
        loadTree();
      } catch (err) {
        alert(`创建失败: ${err}`);
      }
    }
  };

  const getStatus = (node: TreeNode) => statusMap[node.id] || node.status || 'stopped';

  // ── 渲染 ────────────────────────────────────────────────
  const renderTree = (node: TreeNode) => {
    const isFolder = node.type === 'folder';
    const folded = collapsed[node.id] === true;
    const isDragTarget = dragTarget === node.id;

    if (isFolder) {
      return (
        <div key={node.id}>
          <div
            className={styles.folderRow}
            style={isDragTarget ? {
              background: 'rgba(102,126,234,0.15)',
              border: '2px dashed #667eea',
              borderRadius: 6,
              margin: '0 -4px',
              padding: '2px 4px',
            } : undefined}
            onClick={() => toggleFolder(node.id)}
            onMouseEnter={() => {
              if (dragOver) {
                setDragTarget(node.id);
                dragTargetRef.current = node.id;
              }
            }}
            onMouseLeave={() => {
              if (dragOver && dragTarget === node.id) {
                setDragTarget(null);
                dragTargetRef.current = tree?.id || 'Recipes';
              }
            }}
          >
            <span style={{ marginRight: 6 }}>{folded ? '▶' : '▼'}</span>
            <span>{folded ? '📁' : '📂'} {node.name}</span>
            <span style={{ fontSize: 11, color: '#999', marginLeft: 8 }}>
              {(node.children || []).length} 项
            </span>
            <span style={{ fontSize: 9, color: '#667eea', marginLeft: 4, opacity: isDragTarget ? 1 : 0 }}>
              📥 放到这里
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

    // recipe
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
        <p style={{ color: '#6c757d', fontSize: 13 }}>
          拖拽 .py / .kln 文件到此处或文件夹上导入 · 新建命名以 / 结尾创建文件夹
        </p>
      </div>

      {/* 导入提示 */}
      {importMsg && (
        <div style={{
          padding: '8px 16px', marginBottom: 8, borderRadius: 6,
          background: importMsg.includes('失败') ? '#fee2e2' : '#dbeafe',
          color: importMsg.includes('失败') ? '#dc2626' : '#2563eb',
          fontSize: 13,
        }}>
          {importMsg}
          <button onClick={() => setImportMsg(null)}
            style={{ marginLeft: 12, cursor: 'pointer', background: 'none', border: 'none', fontSize: 14 }}>
            ✕
          </button>
        </div>
      )}

      <div className="section" style={{ position: 'relative' }}>
        {/* 拖拽悬停覆盖层 */}
        {dragOver && (
          <div style={{
            position: 'absolute', inset: 0, zIndex: 10,
            border: '3px dashed #667eea', borderRadius: 8,
            background: 'rgba(102,126,234,0.06)',
            display: 'flex', alignItems: 'center', justifyContent: 'center',
            fontSize: 18, color: '#667eea', fontWeight: 500,
            pointerEvents: 'none',
          }}>
            📦 释放以导入配方
          </div>
        )}

        <div className="section-header">
          <div className="section-title">配方树</div>
          <div style={{ display: 'flex', gap: 8 }}>
            <button className="btn btn-sm" onClick={loadTree}>🔄 刷新</button>
            <button className="btn btn-sm" onClick={() => handleNew(tree?.id || 'Recipes')}>
              新建配方
            </button>
            <button className="btn btn-sm" onClick={() => {
              invoke('open_app_dir', { sub: 'Recipes' }).catch(err => alert(`打开失败: ${err}`));
            }}>📂 打开目录</button>
          </div>
        </div>
        {tree && renderTree(tree)}
      </div>
    </>
  );
}
