using System;
using System.Runtime.InteropServices;
using Microsoft.VisualStudio.Shell;

namespace ModelViewerExtension;

[Guid("b6c3a1c9-8919-43ac-bbd9-c7f2f58d7b80")]
public sealed class ModelViewerToolWindow : ToolWindowPane
{
    public ModelViewerToolWindow() : base(null)
    {
        Caption = "Runtime Model Viewer";
        Content = new ModelViewerControl();
    }
}
