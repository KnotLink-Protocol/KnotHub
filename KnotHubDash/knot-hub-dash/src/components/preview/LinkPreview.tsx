// src/components/preview/LinkPreview.tsx
import React from 'react';

interface LinkPreviewProps {
  linkId: string;
}

const LinkPreview: React.FC<LinkPreviewProps> = ({ linkId }) => {
  const details: Record<string, any> = {
    'main-link': { type: '主骨干链路', rate: '12.4 Mbps', latency: '8ms', status: '已连接' },
    'backup-link': { type: '备份链路-电信', rate: '3.2 Mbps', latency: '25ms', status: '波动' },
    'hot-link': { type: '专线热备链路', rate: '0 Mbps', latency: 'N/A', status: '待命' },
  };
  const info = details[linkId] || { type: '未知', rate: '--', latency: '--', status: '--' };

  return (
    <>
      <div className="preview-field"><strong>链路名称</strong> {info.type}</div>
      <div className="preview-field"><strong>实时速率</strong> {info.rate}</div>
      <div className="preview-field"><strong>延迟</strong> {info.latency}</div>
      <div className="preview-field"><strong>状态</strong> {info.status}</div>
      <hr />
      <button className="btn btn-sm" onClick={() => alert(`断开 ${linkId}`)}>断开</button>
      <button className="btn btn-sm" onClick={() => alert(`重连 ${linkId}`)}>重连</button>
    </>
  );
};

export default LinkPreview;