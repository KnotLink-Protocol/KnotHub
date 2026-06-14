export default function Interconnect() {
  const handleRowClick = (linkId: string) => {
    window.dispatchEvent(new CustomEvent('update-preview', { detail: { type: 'link', id: linkId } }));
  };
  return (
    <>
      <div style={{ marginBottom: 16 }}>
        <h1 style={{ fontSize: 22, fontWeight: 500 }}>互联配方</h1>
        <p style={{ color: '#6c757d' }}>链路管理与下载控制</p>
      </div>
      <div className="section">
        <div className="section-header">
          <div className="section-title">互联网运行管理器 · 链路控制</div>
          <button className="btn btn-sm" onClick={() => alert('新建链路（演示）')}>新建链路</button>
        </div>
        <table className="monitor-table">
          <thead><tr><th>链路名称</th><th>状态</th><th>实时速率</th></tr></thead>
          <tbody>
            <tr onClick={() => handleRowClick('main-link')}><td>主骨干链路</td><td><span className="status-badge">已连接</span></td><td>12.4 Mbps</td></tr>
            <tr onClick={() => handleRowClick('backup-link')}><td>备份链路-电信</td><td><span className="status-badge warning">波动</span></td><td>3.2 Mbps</td></tr>
            <tr onClick={() => handleRowClick('hot-link')}><td>专线热备链路</td><td><span className="status-badge">待命</span></td><td>0 Mbps</td></tr>
          </tbody>
        </table>
      </div>
    </>
  );
}