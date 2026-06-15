// src/components/preview/PortPreview.tsx
import React from 'react';

interface PortPreviewProps {
  port: string;
}

const PortPreview: React.FC<PortPreviewProps> = ({ port }) => {
  return (
    <>
      <div className="preview-field"><strong>端口</strong> {port}</div>
      <button className="btn btn-sm" onClick={() => alert(`重新检查端口 ${port}`)}>重新检查</button>
    </>
  );
};

export default PortPreview;