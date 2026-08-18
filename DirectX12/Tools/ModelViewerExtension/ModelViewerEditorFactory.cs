using System;
using System.Runtime.InteropServices;
using Microsoft.VisualStudio;
using Microsoft.VisualStudio.Shell;
using Microsoft.VisualStudio.Shell.Interop;
using IOleServiceProvider = Microsoft.VisualStudio.OLE.Interop.IServiceProvider;

namespace ModelViewerExtension;

[Guid(EditorFactoryGuidString)]
public sealed class ModelViewerEditorFactory : IVsEditorFactory, IDisposable
{
    public const string EditorFactoryGuidString = "e891604d-342d-42f1-b839-f1a124e24152";

    private readonly ModelViewerPackage m_Package;
    private ServiceProvider? m_ServiceProvider;

    public ModelViewerEditorFactory(ModelViewerPackage package)
    {
        m_Package = package;
    }

    public int SetSite(IOleServiceProvider serviceProvider)
    {
        m_ServiceProvider = new ServiceProvider(serviceProvider);
        return VSConstants.S_OK;
    }

    public int Close()
    {
        Dispose();
        return VSConstants.S_OK;
    }

    public int MapLogicalView(ref Guid logicalView, out string? physicalView)
    {
        physicalView = null;
        return logicalView == VSConstants.LOGVIEWID_Primary ? VSConstants.S_OK : VSConstants.E_NOTIMPL;
    }

    public int CreateEditorInstance(
        uint createFlags,
        string documentPath,
        string physicalView,
        IVsHierarchy hierarchy,
        uint itemId,
        IntPtr existingDocumentData,
        out IntPtr documentView,
        out IntPtr documentData,
        out string editorCaption,
        out Guid commandUiGuid,
        out int createDocumentWindowFlags)
    {
        ThreadHelper.ThrowIfNotOnUIThread();
        documentView = IntPtr.Zero;
        documentData = IntPtr.Zero;
        editorCaption = string.Empty;
        commandUiGuid = new Guid(EditorFactoryGuidString);
        createDocumentWindowFlags = 0;

        if ((createFlags & (VSConstants.CEF_OPENFILE | VSConstants.CEF_SILENT)) == 0)
        {
            return VSConstants.E_INVALIDARG;
        }

        if (existingDocumentData != IntPtr.Zero)
        {
            return VSConstants.VS_E_INCOMPATIBLEDOCDATA;
        }

        ModelViewerEditorPane pane = new(m_Package, documentPath);
        documentView = Marshal.GetIUnknownForObject(pane);
        documentData = Marshal.GetIUnknownForObject(pane);
        return VSConstants.S_OK;
    }

    public void Dispose()
    {
        m_ServiceProvider?.Dispose();
        m_ServiceProvider = null;
    }
}
