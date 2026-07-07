import { useState, useEffect, useCallback } from 'react';
import { invoke } from '@tauri-apps/api/core';

interface NodeSummary {
  id: string;
  status: string;
}

export default function Home() {
  const [knotlinkOk, setKnotlinkOk] = useState<boolean | null>(null);
  const [pluginCount, setPluginCount] = useState<number | null>(null);
  const [standaloneCount, setStandaloneCount] = useState<number | null>(null);
  const [recipeCount, setRecipeCount] = useState<number | null>(null);

  const refresh = useCallback(async () => {
    // 服务端口
    try {
      const ok = await invoke<boolean>('check_service_port', { addr: '127.0.0.1:6376' });
      setKnotlinkOk(ok);
    } catch { setKnotlinkOk(false); }

    // 插入式
    try {
      const list = await invoke<NodeSummary[]>('get_plugin_list');
      setPluginCount(list.length);
    } catch { setPluginCount(null); }

    // 独立式
    try {
      const list = await invoke<NodeSummary[]>('get_standalone_list');
      setStandaloneCount(list.length);
    } catch { setStandaloneCount(null); }

    // 配方
    try {
      const tree = await invoke<any>('get_recipe_tree');
      const count = countRecipes(tree);
      setRecipeCount(count);
    } catch { setRecipeCount(null); }
  }, []);

  useEffect(() => { refresh(); }, [refresh]);

  const fmt = (v: number | null) => v === null ? '—' : String(v);

  return (
    <>
      <div style={{ marginBottom: 16 }}>
        <h1 style={{ fontSize: 22, fontWeight: 500 }}>主页</h1>
        <p style={{ color: '#6c757d', marginTop: 4 }}>KnotHub 服务中枢 · 实时总览</p>
      </div>

      {/* 服务状态 */}
      <div className="stats-grid">
        <div className="stat-card">
          <div className="stat-title">KnotLink 服务</div>
          <div className="stat-value" style={{
            color: knotlinkOk === true ? '#2ecc71' : knotlinkOk === false ? '#e74c3c' : '#999'
          }}>
            {knotlinkOk === true ? '在线' : knotlinkOk === false ? '离线' : '检测中'}
          </div>
        </div>
        <div className="stat-card" onClick={() => window.location.hash = '#/nodes'}>
          <div className="stat-title">插入式节点</div>
          <div className="stat-value">{fmt(pluginCount)}</div>
        </div>
        <div className="stat-card" onClick={() => window.location.hash = '#/nodes'}>
          <div className="stat-title">独立式节点</div>
          <div className="stat-value">{fmt(standaloneCount)}</div>
        </div>
        <div className="stat-card" onClick={() => window.location.hash = '#/interconnect'}>
          <div className="stat-title">互联配方</div>
          <div className="stat-value">{fmt(recipeCount)}</div>
        </div>
      </div>

      <div className="section">
        <div className="section-header">
          <div className="section-title">快速操作</div>
        </div>
        <div style={{ display: 'flex', gap: 8 }}>
          <button className="btn" onClick={refresh}>🔄 刷新全部</button>
          <button className="btn" onClick={() => window.location.hash = '#/nodes'}>
            📋 节点管理
          </button>
          <button className="btn" onClick={() => window.location.hash = '#/interconnect'}>
            🐍 配方列表
          </button>
        </div>
      </div>
    </>
  );
}

function countRecipes(node: any): number {
  if (!node) return 0;
  if (node.type === 'recipe') return 1;
  let n = 0;
  if (node.children) {
    for (const c of node.children) n += countRecipes(c);
  }
  return n;
}
