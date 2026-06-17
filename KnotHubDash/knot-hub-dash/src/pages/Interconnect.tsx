import { useState, useEffect, useCallback } from 'react';
import { invoke } from '@tauri-apps/api/core';
import styles from './Interconnect.module.css';

// 节点类型
type NodeType = 'folder' | 'recipe';

interface RecipeData {
  url?: string;
  method?: string;
  headers?: Record<string, string>;
  body?: string;
}

interface TreeNode {
  id: string;
  name: string;
  type: NodeType;
  parentId: string | null;
  recipeData?: RecipeData;
  children?: TreeNode[];
}

// 生成唯一ID（仅用于临时创建，实际 ID 可由后端生成）
const generateId = () => Date.now().toString(36) + Math.random().toString(36).substr(2);

export default function Interconnect() {
  const [nodes, setNodes] = useState<TreeNode[]>([]);
  const [expandedFolders, setExpandedFolders] = useState<Set<string>>(new Set(['root']));
  const [editingNode, setEditingNode] = useState<{ id: string; name: string } | null>(null);
  const [loading, setLoading] = useState(true);

  // ---------- 加载数据 ----------
  const loadData = useCallback(async () => {
    try {
      setLoading(true);
      // 调用后端命令获取树数据
      const data = await invoke<TreeNode[]>('get_interconnect_tree');
      setNodes(data);
    } catch (err) {
      console.error('加载互联配方失败:', err);
      // 如果后端没有数据，可以初始化默认树
      const defaultTree: TreeNode[] = [
        { id: 'root', name: '根目录', type: 'folder', parentId: null },
        { id: 'folder1', name: '生产环境', type: 'folder', parentId: 'root' },
        { id: 'recipe1', name: '主链路', type: 'recipe', parentId: 'folder1', recipeData: { url: 'https://api.example.com/link', method: 'GET' } },
        { id: 'folder2', name: '测试环境', type: 'folder', parentId: 'root' },
        { id: 'recipe2', name: '备份链路', type: 'recipe', parentId: 'folder2', recipeData: { url: 'https://backup.example.com', method: 'POST' } },
      ];
      setNodes(defaultTree);
    } finally {
      setLoading(false);
    }
  }, []);

  // ---------- 保存数据 ----------
  const saveData = useCallback(async (newNodes: TreeNode[]) => {
    try {
      await invoke('save_interconnect_tree', { tree: newNodes });
      setNodes(newNodes);
    } catch (err) {
      console.error('保存互联配方失败:', err);
      alert('保存失败，请检查后端服务');
    }
  }, []);

  useEffect(() => {
    loadData();
  }, [loadData]);

  // ---------- 树操作 ----------
  const buildTree = (items: TreeNode[], parentId: string | null = null): TreeNode[] => {
    return items
      .filter(item => item.parentId === parentId)
      .map(item => ({
        ...item,
        children: buildTree(items, item.id),
      }));
  };

  const treeData = buildTree(nodes, 'root');

  const toggleFolder = (folderId: string) => {
    setExpandedFolders(prev => {
      const newSet = new Set(prev);
      if (newSet.has(folderId)) newSet.delete(folderId);
      else newSet.add(folderId);
      return newSet;
    });
  };

  const handleAdd = async (parentId: string, type: NodeType) => {
    const newNode: TreeNode = {
      id: generateId(), // 如果后端支持生成 ID，可以改为发送临时 ID 或让后端生成
      name: type === 'folder' ? '新建文件夹' : '新建配方',
      type,
      parentId,
      ...(type === 'recipe' ? { recipeData: { url: '', method: 'GET' } } : {}),
    };
    const newNodes = [...nodes, newNode];
    await saveData(newNodes);
    setExpandedFolders(prev => new Set(prev).add(parentId));
    setEditingNode({ id: newNode.id, name: newNode.name });
  };

  const handleDelete = async (nodeId: string) => {
    const toDelete = new Set<string>();
    const collect = (id: string) => {
      toDelete.add(id);
      nodes.filter(n => n.parentId === id).forEach(child => collect(child.id));
    };
    collect(nodeId);
    const newNodes = nodes.filter(n => !toDelete.has(n.id));
    await saveData(newNodes);
    if (editingNode && editingNode.id === nodeId) setEditingNode(null);
  };

  const handleRename = async (nodeId: string, newName: string) => {
    if (!newName.trim()) return;
    const newNodes = nodes.map(n => n.id === nodeId ? { ...n, name: newName.trim() } : n);
    await saveData(newNodes);
    setEditingNode(null);
  };

  const handleEditRecipe = (recipeId: string) => {
    const recipe = nodes.find(n => n.id === recipeId);
    if (recipe && recipe.type === 'recipe') {
      window.dispatchEvent(new CustomEvent('update-preview', {
        detail: { type: 'recipe', id: recipeId, data: recipe.recipeData }
      }));
    }
  };

  // ---------- 渲染 ----------
  const renderTree = (items: TreeNode[], level = 0) => {
    return items.map(item => {
      const isExpanded = expandedFolders.has(item.id);
      const isEditing = editingNode?.id === item.id;
      const hasChildren = item.children && item.children.length > 0;

      return (
        <div key={item.id} style={{ marginLeft: level * 20 }}>
          <div className={styles.treeNode}>
            <div className={styles.treeNodeContent}>
              {item.type === 'folder' && (
                <button className={styles.expandBtn} onClick={() => toggleFolder(item.id)}>
                  {isExpanded ? '📂' : '📁'}
                </button>
              )}
              {item.type === 'recipe' && (
                <span className={styles.recipeIcon}>🔌</span>
              )}
              {isEditing ? (
                <input
                  type="text"
                  defaultValue={item.name}
                  autoFocus
                  onBlur={(e) => handleRename(item.id, e.target.value)}
                  onKeyDown={(e) => {
                    if (e.key === 'Enter') handleRename(item.id, e.currentTarget.value);
                    if (e.key === 'Escape') setEditingNode(null);
                  }}
                  className={styles.editInput}
                />
              ) : (
                <span
                  className={styles.nodeName}
                  onClick={() => item.type === 'recipe' && handleEditRecipe(item.id)}
                >
                  {item.name}
                </span>
              )}
            </div>
            <div className={styles.nodeActions}>
              {item.type === 'folder' && (
                <>
                  <button className="btn btn-sm" onClick={() => handleAdd(item.id, 'folder')}>+文件夹</button>
                  <button className="btn btn-sm" onClick={() => handleAdd(item.id, 'recipe')}>+配方</button>
                </>
              )}
              <button className="btn btn-sm" onClick={() => setEditingNode({ id: item.id, name: item.name })}>✏️</button>
              <button className="btn btn-sm btn-danger" onClick={() => handleDelete(item.id)}>🗑️</button>
            </div>
          </div>
          {item.type === 'folder' && isExpanded && hasChildren && (
            <div className={styles.children}>
              {renderTree(item.children!, level + 1)}
            </div>
          )}
        </div>
      );
    });
  };

  if (loading) return <div className="loading">加载配方树...</div>;

  return (
    <>
      <div style={{ marginBottom: 16 }}>
        <h1 style={{ fontSize: 22, fontWeight: 500 }}>互联配方</h1>
        <p style={{ color: '#6c757d' }}>链路配方管理 · 文件夹分类</p>
      </div>
      <div className="section">
        <div className="section-header">
          <div className="section-title">配方树</div>
          <div className="btn-group">
            <button className="btn btn-sm" onClick={() => handleAdd('root', 'folder')}>新建文件夹</button>
            <button className="btn btn-sm" onClick={() => handleAdd('root', 'recipe')}>新建配方</button>
          </div>
        </div>
        <div className={styles.treeContainer}>
          {renderTree(treeData)}
        </div>
      </div>
    </>
  );
}