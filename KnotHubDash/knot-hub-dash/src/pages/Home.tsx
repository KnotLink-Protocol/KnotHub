export default function Home() {
  return (
    <>
      <div style={{ marginBottom: 16 }}>
        <h1 style={{ fontSize: 22, fontWeight: 500 }}>主页</h1>
        <p style={{ color: '#6c757d', marginTop: 4 }}>服务总览与关键指标</p>
      </div>
      <div className="stats-grid">
        <div className="stat-card"><div className="stat-title">服务实例</div><div className="stat-value">12</div></div>
        <div className="stat-card"><div className="stat-title">链路连接数</div><div className="stat-value">24</div></div>
        <div className="stat-card"><div className="stat-title">双机热备状态</div><div className="stat-value">主→备</div></div>
        <div className="stat-card"><div className="stat-title">下载管理</div><div className="stat-value">4.2 MB/s</div></div>
      </div>
      <div className="section">
        <div className="section-header"><div className="section-title">快速操作</div></div>
        <button className="btn btn-primary" onClick={() => alert('同步操作（演示）')}>触发同步</button>
      </div>
    </>
  );
}