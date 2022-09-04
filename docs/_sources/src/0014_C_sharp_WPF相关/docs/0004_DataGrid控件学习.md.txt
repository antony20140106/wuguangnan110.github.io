# 简述

针对DataGrid进行学习

## 参考

* [WPF的DataGrid用法](https://www.cnblogs.com/Stay627/p/12076084.html)
* [WPF DataGrid 代码参考这里](https://www.jianshu.com/p/6a292017f937)
* [C#将秒数转化为时分秒格式00:00:00](https://blog.csdn.net/qq_16687863/article/details/101035803)
* [WPF中如何移除DataGrid最后一列空列？](https://blog.csdn.net/zhaocg00/article/details/117399806)
* [在WPF数据网格清除值(Clear datagrid values in wpf)](https://www.manongdao.com/article-1137993.html)
* [代码](refers/Marathon_Pace_Calculation)

## 代码分析

首先设置`AutoGenerateColumns="False"` ，表格字段的显示就要靠我们手动去完成了。这个也是数据绑定的重点，因为实际应用中我们大多都是自定义去完成DataGrid的数据绑定。
```C#
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Controls.Primitives;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;

namespace Marathon_Pace_Calculation
{
    /// <summary>
    /// MainWindow.xaml 的交互逻辑
    /// </summary>
    /// 
    public class people
    {
        public string dist { get; set; }
        public string pace { get; set; }
        public string time { get; set; }
    }

    public partial class MainWindow : Window
    {


        ObservableCollection<people> peopleList = new ObservableCollection<people>();
        public MainWindow()
        {
            InitializeComponent();
        }


        //将秒数转化为时分秒
        private string sec_to_hms(int duration)
        {
            TimeSpan ts = new TimeSpan(0, 0, Convert.ToInt32(duration));
            string str = "";
            if (ts.Hours > 0)
            {
                str = String.Format("{0:00}", ts.Hours) + ":" + String.Format("{0:00}", ts.Minutes) + ":" + String.Format("{0:00}", ts.Seconds);
            }
            if (ts.Hours == 0 && ts.Minutes > 0)
            {
                str = "00:" + String.Format("{0:00}", ts.Minutes) + ":" + String.Format("{0:00}", ts.Seconds);
            }
            if (ts.Hours == 0 && ts.Minutes == 0)
            {
                str = "00:00:" + String.Format("{0:00}", ts.Seconds);
            }
            return str;
        }


        //创建dataGrid数据
        private void LoadData(string min, string sec)
        {
            for (int i = 1; i <= 42; ++i)
            {
                peopleList.Add(new people()
                {
                    dist = i.ToString(),
                    pace = min + ":" + sec,
                    time = sec_to_hms((Convert.ToInt32(min)*60 + Convert.ToInt32(sec))*i),
                });
                if (i == 21)
                {
                    peopleList.Add(new people()
                    {
                        dist = 21.0975.ToString(),
                        pace = min + ":" + sec,
                        time = sec_to_hms((Convert.ToInt32(min) * 60 + Convert.ToInt32(sec)) * 210975/10000),
                    });
                }

                if (i == 42)
                {
                    peopleList.Add(new people()
                    {
                        dist = 42.195.ToString(),
                        pace = min + ":" + sec,
                        time = sec_to_hms((Convert.ToInt32(min) * 60 + Convert.ToInt32(sec)) * 42195 / 1000),
                    });
                }
            }
            ((this.FindName("dataGrid1")) as DataGrid).ItemsSource = peopleList;
        }

        private void Button_Click(object sender, RoutedEventArgs e)
        {
            peopleList.Clear();
            dataGrid1.Items.Refresh(); //清除所有表中的数据
            if (sec_value.Text != String.Empty && min_value.Text != String.Empty)
            {
                LoadData(min_value.Text, sec_value.Text);
            }
            else
            {
                MessageBox.Show("请输出正确数值！！！");
            }
        }
    }
}
```

## xmal

```C#
<Window x:Class="Marathon_Pace_Calculation.MainWindow"
        xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation"
        xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml"
        xmlns:d="http://schemas.microsoft.com/expression/blend/2008"
        xmlns:mc="http://schemas.openxmlformats.org/markup-compatibility/2006"
        xmlns:local="clr-namespace:Marathon_Pace_Calculation"
        mc:Ignorable="d"
        Title="Marathon Pace Calculation V1.0    Author : wugangnan   E-Mail : 569542647@qq.com"  Height="587.677" Width="607.507">
    <Grid>
        <Grid.ColumnDefinitions> 
            <ColumnDefinition Width="449*"/>
            <ColumnDefinition Width="343*"/>
        </Grid.ColumnDefinitions>

        //背景色绿色渐变
        <Grid.Background>
            <LinearGradientBrush>
                <GradientStop Color="#FF73A05E" Offset="0.367"/>
                <GradientStop Color="White" Offset="1"/>
                <GradientStop Color="#FF6EBF61" Offset="2"/>
            </LinearGradientBrush>
        </Grid.Background>

        <DataGrid Name="dataGrid1"  AutoGenerateColumns="False" Grid.ColumnSpan="4" Margin="10,48,27,10" HorizontalAlignment="Left" //删除最后一列空列>

            <DataGrid.Columns>
                <DataGridTextColumn Header="公里" Width="130" Binding="{Binding dist}"/>
                <DataGridTextColumn Header="配速" Width="130" Binding="{Binding pace}"/>
                <DataGridTextColumn Header="累计用时" Width="280" Binding="{Binding time}"/>
            </DataGrid.Columns>

 //设置行头颜色
            <DataGrid.ColumnHeaderStyle>
                <Style TargetType="DataGridColumnHeader">
                    <Setter Property="Background">
                        <Setter.Value>
                            <LinearGradientBrush StartPoint="0,0" EndPoint="0,1">
                                <GradientStop Color="White" Offset="1"/>
                                <GradientStop Color="LightBlue" Offset="0.1"/>
                                <GradientStop Color="White" Offset="2"/>
                            </LinearGradientBrush>
                        </Setter.Value>
                    </Setter>
                    <Setter Property="Foreground" Value="Black"/>
                    <Setter Property="FontSize" Value="13" />
                </Style>
            </DataGrid.ColumnHeaderStyle>

        </DataGrid>

        <TextBox Name ="min_value" HorizontalAlignment="Left" Height="23" Margin="48,18,0,0" TextWrapping="Wrap"  VerticalAlignment="Top" Width="40"/>
        <TextBox Name ="sec_value" HorizontalAlignment="Left" Height="23" Margin="111,18,0,0" TextWrapping="Wrap"  VerticalAlignment="Top" Width="44"/>
        <Button Content="计算" HorizontalAlignment="Left" Margin="187,18,0,0" VerticalAlignment="Top" Width="52" Height="23" Click="Button_Click"/>
        <Label Content="分" HorizontalAlignment="Left" Margin="88,18,0,0" VerticalAlignment="Top" RenderTransformOrigin="1.297,0.495" Height="25" Width="22"/>
        <Label Content="秒" HorizontalAlignment="Left" Margin="160,18,0,0" VerticalAlignment="Top" RenderTransformOrigin="1.297,0.495" Height="25" Width="22"/>
        <Label Content="配速：" HorizontalAlignment="Left" Margin="10,18,0,0" VerticalAlignment="Top" RenderTransformOrigin="1.297,0.495" Height="25" Width="38"/>
    </Grid>
</Window>
```

## 如何修改列标题

* [wpf datagrid 列名修改 列标题修改](https://blog.csdn.net/liukun0928/article/details/79886646)

直接修改`dataGrid1.Columns[i].Header`:
```C#
public void addColumn(string title)
    {
        if (title == "Hrate")
        {
            LS.Add("项目");
            LS.Add("HRR%~HRR%");
            LS.Add("最低心率");
            LS.Add("最高心率");
        }
        else
        {
            LS.Add("公里");
            LS.Add("配速");
            LS.Add("累计用时");
        }

        //dataGrid1.Columns.Clear();
        for (int i = 0; i < LS.Count; i++)
        {
            DataGridTextColumn dl = new DataGridTextColumn();
            dataGrid1.Columns[i].Header = LS[i];
        }

    }
```