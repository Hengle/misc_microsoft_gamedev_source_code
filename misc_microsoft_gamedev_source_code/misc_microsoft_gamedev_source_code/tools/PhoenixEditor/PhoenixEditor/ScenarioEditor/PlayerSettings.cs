using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;

using UIHelper;
using SimEditor;
using System.IO;
using EditorCore;

namespace PhoenixEditor.ScenarioEditor
{
   public partial class PlayerSettings : UserControl
   {
      public PlayerSettings()
      {
         TriggerSystemMain.Init();

         InitializeComponent();

         this.PopSettingsList.mListDataObjectType = typeof(PlayerPop);
         PopSettingsList.AddMetaDataForProp("PlayerPop", "PopType", "SimpleEnumeration", TriggerSystemMain.mSimResources.mGameData.mPopTypes.ToArray());
         PopSettingsList.SetTypeEditor("PlayerPop", "PopType", typeof(EnumeratedProperty));
         PopSettingsList.AddMetaDataForProp("PlayerPop", "Max", "Min", 0);
         PopSettingsList.AddMetaDataForProp("PlayerPop", "Max", "Max", 1000);
         PopSettingsList.AddMetaDataForProp("PlayerPop", "Pop", "Min", 0);
         PopSettingsList.AddMetaDataForProp("PlayerPop", "Pop", "Max", 1000);
         PopSettingsList.WrapContents = false;


         PopSettingsList.AnyObjectPropertyChanged += new BasicTypedSuperList.PropertyChanged(PopSettingsList_AnyObjectPropertyChanged);
      }

      void PopSettingsList_AnyObjectPropertyChanged(ObjectEditorControl sender, object selectedObject, HighLevelProperty prop)
      {
         PlayerSettingsChanged();
      }

      private void PlayerSettingsChanged()
      {
         if (CoreGlobals.getEditorMain().mPhoenixScenarioEditor.CheckTopicPermission("Sim") == false)
         {
            return;
         }

      }


      public SimMain SimMain
      {
         set
         {
            mSimMain = value;
            LoadScenarioPlayerData();
         }

      }

      SimMain mSimMain;

      bool mbLoaded = false;
      private void InitPlayerControl()
      {
         if (mbLoaded == true)
            return;

         NumPlayersComboBox.Items.Clear();
         NumPlayersComboBox.DropDownStyle = ComboBoxStyle.DropDownList;
         NumPlayersComboBox.Items.AddRange(new string[] { "1", "2", "3", "4", "5", "6", "7", "8"});
         NumPlayersComboBox.SelectedIndex = 3;


         PlayerPlacementComboBox.Items.Clear();
         PlayerPlacementComboBox.DropDownStyle = ComboBoxStyle.DropDownList;
         PlayerPlacementComboBox.Items.AddRange(new string[] { "Fixed", "Consecutive", "Grouped", "Random" });
         PlayerPlacementComboBox.SelectedIndex = 0;

         PlayerSpacingComboBox.Items.Clear();
         PlayerSpacingComboBox.DropDownStyle = ComboBoxStyle.DropDownList;
         PlayerSpacingComboBox.Items.AddRange(new string[] {"0", "1", "2", "3", "4" });
         PlayerSpacingComboBox.SelectedIndex = 0;

         mbLoaded = true;

      }

      
      private void LoadScenarioPlayerData()
      {
         InitPlayerControl();

         //InitPlayerControl();

         LoadTable();

         BindScenarioPlayerData();
         if (mSimMain.PlayerData.Player.Count != System.Convert.ToInt32(NumPlayersComboBox.Text))
         {
            UpdatePlayers();
         }
         UpdateOtherPlayerLists();
      }




      private void BindScenarioPlayerData()
      {
         //mPlayerPlacementSettingsGrid.SelectedObject = mSimMain.PlayerPlacementData;

         gridControl1.DataSource = mSimMain.PlayerData.Player;

         gridControl1.SingleClickEdit = true;

         AddComboEditor(gridControl1.Columns, "Civ", new string[] { "UNSC", "Covenant" });
         AddComboEditor(gridControl1.Columns, "Team", new string[] { "1", "2", "3", "4" });
         AddComboEditor(gridControl1.Columns, "Color", new string[] { "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12", "13" });

         Dictionary<string,LocString> playerNames = CoreGlobals.getGameResources().mStringTable.mStringsByScenario["PlayerName"];
         Dictionary<string, LocString>.Enumerator it = playerNames.GetEnumerator();
         List<string> playerLocNames = new List<string>();
         while (it.MoveNext())
         {
            if (it.Current.Value.mCategory == "UI")
            {
               playerLocNames.Add(it.Current.Key + "," + it.Current.Value.mString);
            }
         }

         AddComboEditor(gridControl1.Columns, "LocalisedDisplayName", playerLocNames.ToArray());

         List<string> leaderOptions = new List<string>();
         leaderOptions.Add("");//blank choice
         leaderOptions.AddRange(TriggerSystemMain.mSimResources.mLeaders.mLeaders);

         AddComboEditor(gridControl1.Columns, "Leader1", leaderOptions.ToArray());
         AddComboEditor(gridControl1.Columns, "Leader2", new string[]{""});//leaderOptions.ToArray());
         AddComboEditor(gridControl1.Columns, "Leader3", new string[] {""});//leaderOptions.ToArray());



         NumPlayersComboBox.Text = mSimMain.PlayerData.Player.Count.ToString();//.NumberPlayers.ToString();
         PlayerPlacementComboBox.Text = mSimMain.PlayerPlacementData.Type.ToString();
         PlayerSpacingComboBox.Text = mSimMain.PlayerPlacementData.Spacing.ToString();

         //gridControl1.DataRows[1].


      }

      public void AddComboEditor(Xceed.Grid.Collections.ColumnList columns, string columnName, string[] values)
      {
         ComboBox combo = new ComboBox();
         combo.Items.AddRange(values);
         combo.DropDownStyle = ComboBoxStyle.DropDownList;
         columns[columnName].CellEditorManager = new Xceed.Grid.Editors.CellEditorManager(combo, "Text", true, true);
      }
      //public void AddComboEditor2(Xceed.Grid.Collections.ColumnList columns, string columnName, string[] values)
      //{
      //   ComboBox combo = new ComboBox();
      //   //combo.Items.Add(
      //   combo.Items.AddRange(values);
      //   combo.DropDownStyle = ComboBoxStyle.DropDownList;        
      //   columns[columnName].CellEditorManager = new Xceed.Grid.Editors.CellEditorManager(combo, "Text", true, true);
      //}

      public void AddSliderEditor(Xceed.Grid.Collections.ColumnList columns, string columnName)
      {
         TrackBar trackBar = new TrackBar();
         trackBar.Minimum = 0;
         trackBar.Maximum = 10000;
         columns[columnName].CellEditorManager = new Xceed.Grid.Editors.CellEditorManager(trackBar, "Value", false, false);
      }

      public void UpdatePlayers()
      {
         if (mbLoaded == false)
            return;


         int numPlayers = System.Convert.ToInt32(NumPlayersComboBox.Text);
         int difference = numPlayers - mSimMain.PlayerData.Player.Count;

         //mSimMain.PlayerPlacementData.NumberPlayers = (byte)numPlayers;

         if (difference > 0)
         {
            for (int i = 0; i < difference; i++)
            {
               mSimMain.PlayerData.Player.Add(new PlayersPlayer("Player" + (mSimMain.PlayerData.Player.Count + 1).ToString(), mSimMain.PlayerData.Player.Count + 1, (i % 2) + 1));
            }
            BindScenarioPlayerData();

         }
         else if (difference < 0)
         {
            for (int i = 0; i < -difference; i++)
            {
               mSimMain.PlayerData.Player.RemoveAt(mSimMain.PlayerData.Player.Count - 1);
            }
            BindScenarioPlayerData();

         }


         UpdateOtherPlayerLists();
      }
      public void UpdateOtherPlayerLists()
      {
         //player forbid lis
         this.ForbidPlayerListBox.Items.Clear();         
         for(int i=1; i<=mSimMain.PlayerData.Player.Count; i++)
         {
            ForbidPlayerListBox.Items.Add(i.ToString());
         }

         //player pop list
         this.PlayerPopListBox.Items.Clear();
         for (int i = 1; i <= mSimMain.PlayerData.Player.Count; i++)
         {
            PlayerPopListBox.Items.Add(i.ToString());
         }
      }

      private void NumPlayersComboBox_SelectedIndexChanged(object sender, EventArgs e)
      {
         UpdatePlayers();

         PlayerSettingsChanged();

      }
      private void PlayerPlacementComboBox_SelectedIndexChanged(object sender, EventArgs e)
      {
         if (mbLoaded == false)
            return;
         mSimMain.PlayerPlacementData.Type = PlayerPlacementComboBox.Text;

         PlayerSettingsChanged();

      }

      private void PlayerSpacingComboBox_SelectedIndexChanged(object sender, EventArgs e)
      {
         if (mbLoaded == false)
            return;

         mSimMain.PlayerPlacementData.Spacing = System.Convert.ToByte(PlayerSpacingComboBox.Text);

         PlayerSettingsChanged();

      }

      private void ForbidPlayerListBox_SelectedIndexChanged(object sender, EventArgs e)
      {
         int selectedPlayer = System.Convert.ToInt32(ForbidPlayerListBox.SelectedItem.ToString()) - 1;
         //foreach (PlayersPlayer p in mSimMain.PlayerData.Player)
         //{
         //   if(p.

         //}
         PlayersPlayer p = mSimMain.PlayerData.Player[selectedPlayer];

         this.ForbidSquadOptionChooser.Enabled = true;
         this.ForbidTechsOptionChooser.Enabled = true;
         this.ForbidUnitsOptionChooser.Enabled = true;

         TriggerSystemMain.Init();
         ForbidSquadOptionChooser.SetOptions(TriggerSystemMain.mSimResources.mProtoSquadData.mProtoSquadList);
         ForbidSquadOptionChooser.BoundSelectionList = p.mForbidSquads;

         ForbidUnitsOptionChooser.SetOptions(TriggerSystemMain.mSimResources.mObjectTypeData.mObjectTypeList);
         ForbidUnitsOptionChooser.BoundSelectionList = p.mForbidObjects;

         ForbidTechsOptionChooser.SetOptions(TriggerSystemMain.mSimResources.mTechData.mTechList);
         ForbidTechsOptionChooser.BoundSelectionList = p.mForbidTechs;

         ForbidSquadOptionChooser.AllowRepeats = false;
         ForbidSquadOptionChooser.AutoSort = true;
         ForbidUnitsOptionChooser.AllowRepeats = false;
         ForbidUnitsOptionChooser.AutoSort = true;
         ForbidTechsOptionChooser.AllowRepeats = false;
         ForbidTechsOptionChooser.AutoSort = true;


         PlayerSettingsChanged();


      }

      int mNumTeams = 4;  //if this number changes, the table layout should be made dynamic
      public void LoadTable()
      {
         this.tableLayoutPanel1.Controls.Clear();
         Label newLabel;

         if (mSimMain.DiplomacyData.Count == 0)
         {
            DiplomacyXml.InitDiplomacyData(mSimMain.DiplomacyData, mNumTeams);
         }

         for (int i = 0; i <= mNumTeams; i++)
         {

            for (int j = 0; j <= mNumTeams; j++)
            {
               if ((i == 0) && (j == 0))
               {
                  newLabel = new Label();
                  newLabel.Text = "Teams";
                  tableLayoutPanel1.Controls.Add(newLabel, j, i);
               }
               else if (i == 0)
               {
                  newLabel = new Label();
                  newLabel.Text = j.ToString();
                  tableLayoutPanel1.Controls.Add(newLabel, j, i);
               }
               else if (j == 0)
               {
                  newLabel = new Label();
                  newLabel.Text = i.ToString() + " -->";
                  tableLayoutPanel1.Controls.Add(newLabel, j, i);
               }
               else if (j == i)
               {
                  newLabel = new Label();
                  newLabel.Text = "Ally";
                  tableLayoutPanel1.Controls.Add(newLabel, j, i);
               }
               else
               {
                  Label diplomacyLabel = new Label();
                  diplomacyLabel.ForeColor = System.Drawing.Color.Blue;

                  //code to expand # teams...  not tested
                  //if (i > mSimMain.DiplomacyData.Count)
                  //{
                  //   DiplomacyXml dipl = new DiplomacyXml();
                  //   dipl.mTeam = i;
                  //   mSimMain.DiplomacyData.Add(dipl);         
                  //}
                  //if (j > mSimMain.DiplomacyData[i - 1].mTeams.Count)
                  //{
                  //      DiplomacyTeamXml t = new DiplomacyTeamXml();
                  //      t.mId = j;
                  //      t.mStatus = "Neutral";
                  //      mSimMain.DiplomacyData[i - 1].mTeams.Add(t);
                  //      if (j == i)
                  //         t.mStatus = "Ally";
                  //}

                  diplomacyLabel.Text = mSimMain.DiplomacyData[i-1].mTeams[j-1].mStatus;
                  diplomacyLabel.Tag = new Point(i, j);
                  diplomacyLabel.Click += new EventHandler(newLabel_Click);
                  diplomacyLabel.MouseEnter += new EventHandler(diplomacyLabel_MouseEnter);
                  diplomacyLabel.MouseLeave += new EventHandler(diplomacyLabel_MouseLeave);
                  tableLayoutPanel1.Controls.Add(diplomacyLabel, j, i);

               }
            }

         }
      }

      void diplomacyLabel_MouseLeave(object sender, EventArgs e)
      {
         Label label = sender as Label;
         label.Font = new Font(label.Font, FontStyle.Regular);

      }

      void diplomacyLabel_MouseEnter(object sender, EventArgs e)
      {
         Label label = sender as Label;
         label.Font = new Font(label.Font, FontStyle.Underline);
      }

      void newLabel_Click(object sender, EventArgs e)
      {
         Label label = sender as Label;
         Point p = ((Point)label.Tag);

         string newValue = "error";
         if(label.Text == "Enemy")
         {
            newValue = "Neutral";
         }
         else if(label.Text == "Neutral")
         {
            newValue = "Enemy";
         }
         //else if(label.Text == "Ally")
         //{
         //   newValue = "Enemy";
         //}
         label.Text = newValue;

         mSimMain.DiplomacyData[p.X-1].mTeams[p.Y-1].mStatus = newValue;

         //throw new Exception("The method or operation is not implemented.");

         PlayerSettingsChanged();

      }

      private void PlayerPopListBox_SelectedIndexChanged(object sender, EventArgs e)
      {
         int selectedPlayer = System.Convert.ToInt32(PlayerPopListBox.SelectedItem.ToString()) - 1; 
         PlayersPlayer p = mSimMain.PlayerData.Player[selectedPlayer];         
         this.PopSettingsList.ObjectList = p.mPlayerPop;


         PlayerSettingsChanged();
      }


   }

  
}
