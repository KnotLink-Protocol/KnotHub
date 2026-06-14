export default function Settings() {
  return (
    <>
      <div style={{ marginBottom: 16 }}>
        <h1 style={{ fontSize: 22, fontWeight: 500 }}>设置</h1>
        <p style={{ color: '#6c757d' }}>热备配置与系统参数</p>
      </div>
      <div className="section">
        <div className="section-header">
          <div className="section-title">双机热备配置</div>
          <button className="btn btn-primary btn-sm" onClick={() => alert('保存配置')}>保存配置</button>
        </div>
        <div style={{ display: 'flex', gap: 24, flexWrap: 'wrap' }}>
          <div style={{ flex: 1 }}>
            <div style={{ fontWeight: 500, marginBottom: 8 }}>热备实体</div>
            <div>主实体: 10.2.10.101 | 心跳 active</div>
            <div>备实体: 10.2.10.102 | 实时同步 (延迟 &lt; 5ms)</div>
            <div>仲裁策略: 自动故障切换 / 手动干预</div>
          </div>
          <div style={{ flex: 1 }}>
            <div style={{ fontWeight: 500, marginBottom: 8 }}>全局参数</div>
            <div>心跳间隔: 2秒</div>
            <div>数据同步模式: 实时</div>
            <div>断线重试次数: 3次</div>
          </div>
        </div>
        <hr />
        <div>
          <button className="btn btn-sm" onClick={() => alert('导入配置')}>导入配置</button>
          <button className="btn btn-sm" onClick={() => alert('导出热备配置')}>导出热备配置</button>
          <button className="btn btn-sm" onClick={() => alert('全量下发')}>全量下发</button>
        </div>
      </div>
    </>
  );
}