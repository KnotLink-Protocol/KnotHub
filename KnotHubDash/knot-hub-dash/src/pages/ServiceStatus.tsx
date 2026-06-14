export default function ServiceStatus() {
  const handleRowClick = (port: string) => {
    window.dispatchEvent(new CustomEvent('update-preview', { detail: { type: 'port', id: port } }));
  };
  return (
    <>
      <div style={{ marginBottom: 16 }}>
        <h1 style={{ fontSize: 22, fontWeight: 500 }}>服务状态</h1>
        <p style={{ color: '#6c757d' }}>端口监控与服务实例健康度</p>
      </div>
      <div className="section">
        <div className="section-header"><div className="section-title">物理端口检查</div></div>
        <table className="monitor-table">
          <thead><tr><th>端口</th><th>服务</th><th>状态</th></tr></thead>
          <tbody>
            <tr onClick={() => handleRowClick('8080')}><td>8080</td><td>Web</td><td><span className="status-badge">正常</span></td></tr>
            <tr onClick={() => handleRowClick('8443')}><td>8443</td><td>API</td><td><span className="status-badge">正常</span></td></tr>
            <tr onClick={() => handleRowClick('3306')}><td>3306</td><td>DB</td><td><span className="status-badge warning">高延迟</span></td></tr>
          </tbody>
        </table>
      </div>
    </>
  );
}