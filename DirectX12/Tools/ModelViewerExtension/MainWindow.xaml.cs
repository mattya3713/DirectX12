using System.IO;
using System.Numerics;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Media.Media3D;
using Microsoft.Win32;

namespace ModelViewerExtension;

public partial class MainWindow : Window
{
    public MainWindow()
    {
        InitializeComponent();
        App.Current.ShutdownMode = ShutdownMode.OnMainWindowClose;
    }

    private void OpenFileClick(object sender, RoutedEventArgs e)
    {
        OpenFileDialog dialog = new()
        {
            Filter = "Runtime model|*.mskn;*.mmat|Skin model|*.mskn|Material|*.mmat|All files|*.*",
            CheckFileExists = true,
        };
        if (dialog.ShowDialog(this) != true) { return; }
        try
        {
            if (string.Equals(Path.GetExtension(dialog.FileName), ".mmat", StringComparison.OrdinalIgnoreCase)) { ShowMaterial(dialog.FileName); }
            else { ShowModel(dialog.FileName); }
        }
        catch (Exception exception) when (exception is IOException or UnauthorizedAccessException or RuntimeFormatException)
        {
            ClearModel();
            StatusText.Text = exception.Message;
            MessageBox.Show(this, exception.Message, "Runtime Model Viewer", MessageBoxButton.OK, MessageBoxImage.Error);
        }
    }

    private void ShowMaterial(string filePath)
    {
        RuntimeMaterial material = RuntimeFormatReader.ReadMmat(filePath);
        ClearModel();
        FileNameText.Text = filePath;
        SummaryText.Text = "MMAT material\n" + FormatMaterial(material);
        MaterialsList.ItemsSource = new[] { CreateMaterialPanel(material, filePath) };
        StatusText.Text = "MMAT version 2/3 loaded and validated.";
    }

    private void ShowModel(string filePath)
    {
        RuntimeSkinModel model = RuntimeFormatReader.ReadMskn(filePath);
        ClearModel();
        FileNameText.Text = filePath;
        SummaryText.Text = $"MSKN v4\nVertices: {model.Vertices.Count:N0}\nIndices: {model.Indices.Count:N0}\nBones: {model.Bones.Count:N0}\nSkin slots: {model.SkinSlots.Count:N0}\nSubmeshes: {model.Submeshes.Count:N0}";
        List<FrameworkElement> materialPanels = new();
        for (int i = 0; i < model.Submeshes.Count; ++i)
        {
            RuntimeMaterial? material = model.Materials[i];
            materialPanels.Add(material is null
                ? new TextBlock { Text = $"{i}: {model.Submeshes[i].MaterialPath} (missing)", TextWrapping = TextWrapping.Wrap, Margin = new Thickness(0, 2, 0, 2) }
                : CreateMaterialPanel(material, model.Submeshes[i].MaterialPath));
        }
        MaterialsList.ItemsSource = materialPanels;
        ModelVisual.Content = CreateModelGeometry(model);
        StatusText.Text = $"Loaded and validated. FrontComposite opacity: {model.FrontCompositeOpacity:0.###}, max distance: {model.FrontCompositeMaxDistance:0.###}.";
    }

    private Model3DGroup CreateModelGeometry(RuntimeSkinModel model)
    {
        Point3DCollection positions = new();
        Vector3DCollection normals = new();
        PointCollection textureCoordinates = new();
        foreach (RuntimeSkinVertex vertex in model.Vertices)
        {
            Vector3 position = ApplyBindPose(vertex, model);
            positions.Add(new Point3D(position.X, position.Y, -position.Z));
            normals.Add(new Vector3D(vertex.Normal.X, vertex.Normal.Y, -vertex.Normal.Z));
            textureCoordinates.Add(new System.Windows.Point(vertex.UV.X, 1.0 - vertex.UV.Y));
        }
        Model3DGroup group = new();
        int index_offset = 0;
        foreach (int submesh_index in Enumerable.Range(0, model.Submeshes.Count))
        {
            RuntimeSkinSubmesh submesh = model.Submeshes[submesh_index];
            MeshGeometry3D mesh = new() { Positions = positions, Normals = normals, TextureCoordinates = textureCoordinates };
            for (int index = 0; index < submesh.IndexCount; ++index) { mesh.TriangleIndices.Add(checked((int)model.Indices[index_offset + index])); }
            RuntimeMaterial? runtime_material = model.Materials[submesh_index];
            DiffuseMaterial material = runtime_material is null
                ? new DiffuseMaterial(new SolidColorBrush(Color.FromRgb(210, 210, 210)))
                : CreateDiffuseMaterial(runtime_material);
            group.Children.Add(new GeometryModel3D(mesh, material) { BackMaterial = material });
            index_offset += checked((int)submesh.IndexCount);
        }
        return group;
    }

    private static Vector3 ApplyBindPose(RuntimeSkinVertex vertex, RuntimeSkinModel model)
    {
        Vector3 result = Vector3.Zero;
        float total = 0;
        for (int i = 0; i < vertex.BoneWeights.Length; ++i)
        {
            float weight = vertex.BoneWeights[i];
            if (weight <= 0.00001f) { continue; }
            int slotIndex = vertex.BoneIndices[i];
            if (slotIndex >= model.SkinSlots.Count) { continue; }
            Matrix4x4 world = GetBindWorld(model, model.SkinSlots[slotIndex].BoneIndex);
            result += Vector3.Transform(vertex.Position, model.SkinSlots[slotIndex].OffsetMatrix * world) * weight;
            total += weight;
        }
        return total > 0.00001f ? result / total : vertex.Position;
    }

    private static Matrix4x4 GetBindWorld(RuntimeSkinModel model, int boneIndex)
    {
        Matrix4x4 world = Matrix4x4.CreateScale(model.Bones[boneIndex].BindScale) * Matrix4x4.CreateFromQuaternion(model.Bones[boneIndex].BindRotation) * Matrix4x4.CreateTranslation(model.Bones[boneIndex].BindPosition);
        int parent = model.Bones[boneIndex].ParentIndex;
        return parent >= 0 ? world * GetBindWorld(model, parent) : world;
    }

    private FrameworkElement CreateMaterialPanel(RuntimeMaterial material, string sourcePath)
    {
        StackPanel panel = new() { Margin = new Thickness(0, 2, 0, 8) };
        panel.Children.Add(new TextBlock { Text = sourcePath, FontWeight = FontWeights.SemiBold, TextWrapping = TextWrapping.Wrap });
        panel.Children.Add(new TextBlock { Text = FormatMaterial(material), TextWrapping = TextWrapping.Wrap });
        return panel;
    }

    private static string FormatMaterial(RuntimeMaterial material) => $"Diffuse: {material.Diffuse.X:0.###}, {material.Diffuse.Y:0.###}, {material.Diffuse.Z:0.###}, {material.Diffuse.W:0.###}\nSpecular: {material.Specular.X:0.###}, {material.Specular.Y:0.###}, {material.Specular.Z:0.###} (power {material.SpecularPower:0.###})\nAmbient: {material.Ambient.X:0.###}, {material.Ambient.Y:0.###}, {material.Ambient.Z:0.###}\nToon: {material.UseToonMap}, Sphere: {material.UseSphereMap}\nBase: {material.BaseColorTexturePath}\nNormal: {material.NormalMapTexturePath}\nToon texture: {material.ToonTexturePath}\nSphere texture: {material.SphereTexturePath}";

    private static DiffuseMaterial CreateDiffuseMaterial(RuntimeMaterial material)
    {
        Color color = Color.FromArgb((byte)(Math.Clamp(material.Diffuse.W, 0, 1) * 255), (byte)(Math.Clamp(material.Diffuse.X, 0, 1) * 255), (byte)(Math.Clamp(material.Diffuse.Y, 0, 1) * 255), (byte)(Math.Clamp(material.Diffuse.Z, 0, 1) * 255));
        Brush brush = new SolidColorBrush(color);
        string texture = ResolveTexturePath(material.BaseColorTexturePath);
        if (File.Exists(texture)) { brush = new ImageBrush(new BitmapImage(new Uri(texture))) { Stretch = Stretch.Fill }; }
        return new DiffuseMaterial(brush);
    }

    private static string ResolveTexturePath(string path)
    {
        if (string.IsNullOrWhiteSpace(path)) { return string.Empty; }
        string normalized = path.Replace('/', Path.DirectorySeparatorChar);
        DirectoryInfo? directory = new(AppContext.BaseDirectory);
        while (directory is not null)
        {
            string candidate = Path.Combine(directory.FullName, normalized);
            if (File.Exists(candidate)) { return candidate; }
            directory = directory.Parent;
        }
        return string.Empty;
    }

    private void ClearModel()
    {
        ModelVisual.Content = null;
        MaterialsList.ItemsSource = null;
        SummaryText.Text = string.Empty;
    }
}
