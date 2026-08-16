using System;
using System.Runtime.InteropServices;
using System.Threading;
using System.Threading.Tasks;
using Microsoft.VisualStudio.Shell;

namespace ModelViewerExtension;

[PackageRegistration(UseManagedResourcesOnly = true, AllowsBackgroundLoading = true)]
[InstalledProductRegistration("Runtime Model Viewer", "MSKN/MMAT viewer", "1.0.4")]
[ProvideMenuResource("ModelViewer.CTMENU", 1)]
[ProvideToolWindow(typeof(ModelViewerToolWindow))]
[Guid(ModelViewerPackage.PackageGuidString)]
public sealed class ModelViewerPackage : AsyncPackage
{
    public const string PackageGuidString = "4c3a16d8-ecaa-48d9-8b67-c3f6d8e7235b";

    protected override async Task InitializeAsync(CancellationToken cancellationToken, IProgress<ServiceProgressData> progress)
    {
        await JoinableTaskFactory.SwitchToMainThreadAsync(cancellationToken);
        await ShowModelViewerCommand.InitializeAsync(this);
    }
}
