// import { useEffect } from 'react';

export default function Nodes() {
  const handleRowClick = (nodeId: string) => {
    window.dispatchEvent(new CustomEvent('update-preview', { detail: { type: 'node', id: nodeId } }));
  };

  return (
    <>
      <div style={{ marginBottom: 16 }}>
        <h1 style={{ fontSize: 22, fontWeight: 500 }}>节点列表</h1>
        <p style={{ color: '#6c757d' }}>服务节点管理 · 插入式结构</p>
      </div>
      <div className="section">
        <div className="section-header">
          <div className="section-title">节点管理</div>
          <button className="btn btn-sm" onClick={() => alert('安装插件（演示）')}>安装插件</button>
        </div>
        <table className="monitor-table">
          <thead><tr><th>节点ID</th><th>角色</th><th>状态</th><th>热备角色</th></tr></thead>
          <tbody>
            <tr onClick={() => handleRowClick('node-01')}><td>node-01</td><td>主控</td><td><span className="status-badge">运行中</span></td><td>主节点</td></tr>
            <tr onClick={() => handleRowClick('node-02')}><td>node-02</td><td>工作节点</td><td><span className="status-badge">运行中</span></td><td>热备从机</td></tr>
            <tr onClick={() => handleRowClick('node-03')}><td>node-03</td><td>备份节点</td><td><span className="status-badge warning">停止</span></td><td>待命</td></tr>
          </tbody>
        </table>
      </div>
    </>
  );
}