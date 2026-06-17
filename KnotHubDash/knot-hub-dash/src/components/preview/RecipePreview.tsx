import React from 'react';

interface RecipePreviewProps {
  data: {
    id: string;
    name: string;
    author?: string;
    description?: string;
    nodeList?: string[];
    version?: string;
    recipeData?: {
      url?: string;
      method?: string;
      headers?: Record<string, string>;
      body?: string;
    };
  };
}

const RecipePreview: React.FC<RecipePreviewProps> = ({ data }) => {
  return (
    <div className="recipe-preview">
      <div className="preview-field">
        <strong>配方名</strong> {data.name}
      </div>
      <div className="preview-field">
        <strong>作者</strong> {data.author || '未知'}
      </div>
      <div className="preview-field">
        <strong>描述</strong> {data.description || '无描述'}
      </div>
      <div className="preview-field">
        <strong>关联节点</strong> 
        {data.nodeList && data.nodeList.length > 0 
          ? data.nodeList.join(', ') 
          : '无'}
      </div>
      <div className="preview-field">
        <strong>版本</strong> {data.version || '未指定'}
      </div>
      {data.recipeData && (
        <>
          <hr />
          <div className="preview-field">
            <strong>URL</strong> {data.recipeData.url || '-'}
          </div>
          <div className="preview-field">
            <strong>方法</strong> {data.recipeData.method || '-'}
          </div>
          {data.recipeData.headers && (
            <div className="preview-field">
              <strong>请求头</strong> {JSON.stringify(data.recipeData.headers)}
            </div>
          )}
          {data.recipeData.body && (
            <div className="preview-field">
              <strong>请求体</strong> {data.recipeData.body}
            </div>
          )}
        </>
      )}
      <hr />
      <button className="btn btn-sm" onClick={() => alert(`编辑配方 ${data.name}`)}>编辑</button>
      <button className="btn btn-sm btn-danger" onClick={() => alert(`删除配方 ${data.name}`)}>删除</button>
    </div>
  );
};

export default RecipePreview;