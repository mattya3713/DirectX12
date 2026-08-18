using System;
using System.Runtime.InteropServices;
using System.Threading;
using System.Threading.Tasks;
using Microsoft.VisualStudio.Shell;

namespace ModelViewerExtension;

[PackageRegistration(UseManagedResourcesOnly = true, AllowsBackgroundLoading = true)]
[InstalledProductRegistration("Runtime Model Viewer", "MSKN/MMAT viewer", "1.0.12")]
[ProvideMenuResource("ModelViewer.CTMENU", 1)]
[ProvideToolWindow(typeof(ModelViewerToolWindow))]
[ProvideEditorExtension(typeof(ModelViewerEditorFactory), ".mskn", 0x100)]
[ProvideEditorExtension(typeof(ModelViewerEditorFactory), ".mmat", 0x100)]
[ProvideAutoLoad(Microsoft.VisualStudio.Shell.Interop.UIContextGuids80.NoSolution, PackageAutoLoadFlags.BackgroundLoad)]
[ProvideAutoLoad(Microsoft.VisualStudio.Shell.Interop.UIContextGuids80.SolutionExists, PackageAutoLoadFlags.BackgroundLoad)]
[Guid(ModelViewerPackage.PackageGuidString)]
public sealed class ModelViewerPackage : AsyncPackage
{
    public const string PackageGuidString = "4c3a16d8-ecaa-48d9-8b67-c3f6d8e7235b";

    protected override async Task InitializeAsync(CancellationToken cancellationToken, IProgress<ServiceProgressData> progress)
    {
        await JoinableTaskFactory.SwitchToMainThreadAsync(cancellationToken);
        RegisterEditorFactory(new ModelViewerEditorFactory(this));
        await ShowModelViewerCommand.InitializeAsync(this);
    }
}
