using System;
using System.ComponentModel.Design;
using System.Threading.Tasks;
using Microsoft.VisualStudio.Shell;

namespace ModelViewerExtension;

internal sealed class ShowModelViewerCommand
{
    public const int CommandId = 0x0100;
    public static readonly Guid CommandSet = new("a7e33d45-cf1f-4f71-9dc0-34d4f3348e3b");

    private readonly AsyncPackage m_Package;

    private ShowModelViewerCommand(AsyncPackage package, OleMenuCommandService commandService)
    {
        m_Package = package;
        CommandID command_id = new(CommandSet, CommandId);
        commandService.AddCommand(new MenuCommand(Execute, command_id));
    }

    public static async Task InitializeAsync(AsyncPackage package)
    {
        OleMenuCommandService? command_service = await package.GetServiceAsync(typeof(IMenuCommandService)) as OleMenuCommandService;
        if (command_service is not null) { _ = new ShowModelViewerCommand(package, command_service); }
    }

    private void Execute(object sender, EventArgs e)
    {
        ThreadHelper.ThrowIfNotOnUIThread();
        _ = m_Package.ShowToolWindowAsync(typeof(ModelViewerToolWindow), 0, true, m_Package.DisposalToken);
    }
}
