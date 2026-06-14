export default function Nodes() {
  const nodesData = [
    { id: 'node-01', role: '主控', status: '运行中', hotRole: '主节点', statusClass: 'status-badge' },
    { id: 'node-02', role: '工作节点', status: '运行中', hotRole: '热备从机', statusClass: 'status-badge' },
    { id: 'node-03', role: '备份节点', status: '停止', hotRole: '待命', statusClass: 'status-badge warning' },
  ];

  const handleItemClick = (nodeId: string) => {
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
        <div className="nodes-list">
          {nodesData.map(node => (
            <div
              key={node.id}
              className="node-item"
              onClick={() => handleItemClick(node.id)}
            >
              <div className="node-item-content">
                <div className="node-info">
                  <span className="node-id">{node.id}</span>
                  <span className="node-role">{node.role}</span>
                </div>
                <div className="node-detail">
                  <span className={node.statusClass}>{node.status}</span>
                  <span className="node-hot-role">热备角色: {node.hotRole}</span>
                </div>
              </div>
            </div>
          ))}
        </div>
      </div>
    </>
  );
}