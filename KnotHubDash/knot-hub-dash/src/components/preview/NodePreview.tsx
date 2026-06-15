// src/components/preview/NodePreview.tsx
import React from 'react';

interface NodePreviewProps {
  nodeId: string;
}

const NodePreview: React.FC<NodePreviewProps> = ({ nodeId }) => {
  // 模拟数据，后续可改为 invoke 获取
  const details: Record<string, any> = {
    'node-01': { ip: '10.2.10.101', cpu: '12%', mem: '3.2/8 GB', role: '主节点', lastSeen: '2025-03-21 14:32:05' },
    'node-02': { ip: '10.2.10.102', cpu: '34%', mem: '5.1/8 GB', role: '热备从机', lastSeen: '2025-03-21 14:32:01' },
    'node-03': { ip: '10.2.10.103', cpu: '0%', mem: 'N/A', role: '待命', lastSeen: '2025-03-21 10:15:22' },
  };
  const info = details[nodeId] || { ip: '--', cpu: '--', mem: '--', role: '--', lastSeen: '--' };

  const handleRestart = () => alert(`[演示] 重启节点 ${nodeId}`);
  const handleFailover = () => alert(`[演示] 热备切换 ${nodeId}`);

  return (
    <>
      <div className="preview-field"><strong>节点ID</strong> {nodeId}</div>
      <div className="preview-field"><strong>IP地址</strong> {info.ip}</div>
      <div className="preview-field"><strong>角色</strong> {info.role}</div>
      <div className="preview-field"><strong>CPU使用</strong> {info.cpu}</div>
      <div className="preview-field"><strong>内存使用</strong> {info.mem}</div>
      <div className="preview-field"><strong>最后心跳</strong> {info.lastSeen}</div>
      <hr />
      <button className="btn btn-sm" onClick={handleRestart}>重启节点</button>
      <button className="btn btn-sm" onClick={handleFailover}>热备切换</button>
    </>
  );
};

export default NodePreview;