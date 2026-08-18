using System;
using System.Runtime.InteropServices;
using Microsoft.VisualStudio;
using Microsoft.VisualStudio.Shell;
using Microsoft.VisualStudio.Shell.Interop;

namespace ModelViewerExtension;

[ComVisible(true)]
[Guid("67fb2a46-b6a5-4254-9d64-dc0f8a0a43bf")]
public sealed class ModelViewerEditorPane : WindowPane, IVsPersistDocData
{
    private readonly ModelViewerControl m_Control;
    private string m_FilePath;

    public ModelViewerEditorPane(ModelViewerPackage package, string filePath) : base(package)
    {
        m_FilePath = filePath;
        m_Control = new ModelViewerControl();
        Content = m_Control;
        m_Control.LoadFile(filePath);
    }

    int IVsPersistDocData.Close() => VSConstants.S_OK;

    int IVsPersistDocData.GetGuidEditorType(out Guid classId)
    {
        classId = new Guid(ModelViewerEditorFactory.EditorFactoryGuidString);
        return VSConstants.S_OK;
    }

    int IVsPersistDocData.IsDocDataDirty(out int isDirty)
    {
        isDirty = 0;
        return VSConstants.S_OK;
    }

    int IVsPersistDocData.IsDocDataReloadable(out int isReloadable)
    {
        isReloadable = 1;
        return VSConstants.S_OK;
    }

    int IVsPersistDocData.LoadDocData(string documentPath)
    {
        m_FilePath = documentPath;
        m_Control.LoadFile(documentPath);
        return VSConstants.S_OK;
    }

    int IVsPersistDocData.OnRegisterDocData(uint documentCookie, IVsHierarchy hierarchy, uint itemId) => VSConstants.S_OK;

    int IVsPersistDocData.ReloadDocData(uint flags)
    {
        m_Control.LoadFile(m_FilePath);
        return VSConstants.S_OK;
    }

    int IVsPersistDocData.RenameDocData(uint attributes, IVsHierarchy hierarchy, uint itemId, string newDocumentPath)
    {
        m_FilePath = newDocumentPath;
        m_Control.LoadFile(newDocumentPath);
        return VSConstants.S_OK;
    }

    int IVsPersistDocData.SaveDocData(VSSAVEFLAGS saveFlags, out string newDocumentPath, out int saveCanceled)
    {
        newDocumentPath = m_FilePath;
        saveCanceled = 0;
        return VSConstants.S_OK;
    }

    int IVsPersistDocData.SetUntitledDocPath(string documentPath) => VSConstants.E_NOTIMPL;
}
