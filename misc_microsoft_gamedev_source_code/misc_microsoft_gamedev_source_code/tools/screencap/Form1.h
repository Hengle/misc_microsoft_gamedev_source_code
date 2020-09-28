#pragma once

#include "XenonInterface.h"
#include "screencapServer.h"

namespace screencap {

	using namespace System;
	using namespace System::ComponentModel;
	using namespace System::Collections;
	using namespace System::Windows::Forms;
	using namespace System::Data;
	using namespace System::Drawing;
	using namespace System::Runtime::InteropServices; 
	using namespace System::IO;

	/// <summary>
	/// Summary for Form1
	///
	/// WARNING: If you change the name of this class, you will need to change the
	///          'Resource File Name' property for the managed resource compiler tool
	///          associated with all .resx files this class depends on.  Otherwise,
	///          the designers will not be able to interact properly with localized
	///          resources associated with this form.
	/// </summary>
	public ref class Form1 : public System::Windows::Forms::Form
	{
	public:
		Form1(void)
		{
			InitializeComponent();
			
			init();
		}

	protected:
		/// <summary>
		/// Clean up any resources being used.
		/// </summary>
		~Form1()
		{
			if (components)
			{
				delete components;
			}
			
			deinit();
		}
		
   private:
      void init(void)
      {
         try
         {
            XenonInterface::init();
            
            if (XenonInterface::mCurrentConsole != nullptr)
            {
               for each (String^ name in XenonInterface::mXboxManager->Consoles)
               {
                  if (!ConsoleNameDropdown->Items->Contains(name))
                     ConsoleNameDropdown->Items->Add(name);
               }
               
               ConsoleNameDropdown->Text = XenonInterface::mXboxManager->DefaultConsole;
            }
         }
         catch (Exception^)
         {
            System::Diagnostics::Debug::Print("Failed initializing xenon interface");
         }
         
         if (!BScreenCapServer::init())
         {
            System::Diagnostics::Debug::Print("Failed initializing screencap server");
         }
         
         if (BScreenCapServer::getInitialized())
         {
            for (UInt32 i = 0; i < BScreenCapServer::getNumServerAddresses(); i++)
            {
               String^ address = Marshal::PtrToStringAnsi(static_cast<IntPtr>( (char*)BScreenCapServer::getServerAddress(i) ));
               ServerIPDropDown->Items->Add(address);
               
               if (!i)
                  ServerIPDropDown->Text = address;
            }
         }
      }
      
      void deinit(void)
      {
         XenonInterface::destroy();
         
         BScreenCapServer::deinit();
      }
      
      void setWaiting(void)
      {
         Cursor = Cursors::WaitCursor;
      }

      void resetWaiting(void)
      {
         this->Cursor = Cursors::Arrow;
      }
      
      void startXboxViewer(void)
      {
         if (XenonInterface::openConsole(ConsoleNameDropdown->Text) != XenonInterface::eErrorType::cOK)
         {
            MessageBox::Show("Failed opening console: %s" + ConsoleNameDropdown->Text, "Error", MessageBoxButtons::OK, MessageBoxIcon::Exclamation);
            return;
         }

         try
         {
            array<String^>^ files = Directory::GetFiles("xboxTest", "*", SearchOption::AllDirectories);

            for each (String^ filename in files)
            {
               if (filename->Contains(".bat"))
                  continue;

               String^ dstFilename("e:\\" + filename);

               if (XenonInterface::sendFile(filename, dstFilename, true) != XenonInterface::eErrorType::cOK)
               {
                  String^ msg;
                  msg->Format("Failed copying file {0} to {1}", filename, dstFilename);

                  MessageBox::Show(msg, "Error", MessageBoxButtons::OK, MessageBoxIcon::Exclamation);
                  break;
               }
            }
         }
         catch (Exception^)
         {
         }

         XenonInterface::reboot("e:\\xboxTest\\xboxTestP.xex", "e:\\xboxTest", ServerIPDropDown->Text, XboxRebootFlags::Title);
      }
   		
		
   private: System::Windows::Forms::NotifyIcon^  notifyIcon1;
   private: System::Windows::Forms::ComboBox^  ConsoleNameDropdown;

   private: System::Windows::Forms::Label^  label1;
   private: System::Windows::Forms::Button^  StartButton;

   private: System::Windows::Forms::TrackBar^  UpdateRateTrackBar;

   private: System::Windows::Forms::Label^  label2;
   private: System::Windows::Forms::Label^  label3;
   private: System::Windows::Forms::ComboBox^  ServerIPDropDown;



   protected: 
   private: System::ComponentModel::IContainer^  components;

	private:
		/// <summary>
		/// Required designer variable.
		/// </summary>


#pragma region Windows Form Designer generated code
		/// <summary>
		/// Required method for Designer support - do not modify
		/// the contents of this method with the code editor.
		/// </summary>
		void InitializeComponent(void)
		{
         this->components = (gcnew System::ComponentModel::Container());
         System::ComponentModel::ComponentResourceManager^  resources = (gcnew System::ComponentModel::ComponentResourceManager(Form1::typeid));
         this->notifyIcon1 = (gcnew System::Windows::Forms::NotifyIcon(this->components));
         this->ConsoleNameDropdown = (gcnew System::Windows::Forms::ComboBox());
         this->label1 = (gcnew System::Windows::Forms::Label());
         this->StartButton = (gcnew System::Windows::Forms::Button());
         this->UpdateRateTrackBar = (gcnew System::Windows::Forms::TrackBar());
         this->label2 = (gcnew System::Windows::Forms::Label());
         this->label3 = (gcnew System::Windows::Forms::Label());
         this->ServerIPDropDown = (gcnew System::Windows::Forms::ComboBox());
         (cli::safe_cast<System::ComponentModel::ISupportInitialize^  >(this->UpdateRateTrackBar))->BeginInit();
         this->SuspendLayout();
         // 
         // notifyIcon1
         // 
         this->notifyIcon1->Icon = (cli::safe_cast<System::Drawing::Icon^  >(resources->GetObject(L"notifyIcon1.Icon")));
         this->notifyIcon1->Text = L"screencap";
         this->notifyIcon1->Visible = true;
         // 
         // ConsoleNameDropdown
         // 
         this->ConsoleNameDropdown->FormattingEnabled = true;
         this->ConsoleNameDropdown->Location = System::Drawing::Point(92, 12);
         this->ConsoleNameDropdown->Name = L"ConsoleNameDropdown";
         this->ConsoleNameDropdown->Size = System::Drawing::Size(240, 21);
         this->ConsoleNameDropdown->TabIndex = 0;
         // 
         // label1
         // 
         this->label1->AutoSize = true;
         this->label1->Location = System::Drawing::Point(10, 15);
         this->label1->Name = L"label1";
         this->label1->Size = System::Drawing::Size(58, 13);
         this->label1->TabIndex = 1;
         this->label1->Text = L"Console IP";
         // 
         // StartButton
         // 
         this->StartButton->Location = System::Drawing::Point(129, 151);
         this->StartButton->Name = L"StartButton";
         this->StartButton->Size = System::Drawing::Size(102, 56);
         this->StartButton->TabIndex = 2;
         this->StartButton->Text = L"Start";
         this->StartButton->UseVisualStyleBackColor = true;
         this->StartButton->Click += gcnew System::EventHandler(this, &Form1::StartButton_Click);
         // 
         // UpdateRateTrackBar
         // 
         this->UpdateRateTrackBar->Location = System::Drawing::Point(92, 91);
         this->UpdateRateTrackBar->Name = L"UpdateRateTrackBar";
         this->UpdateRateTrackBar->Size = System::Drawing::Size(240, 42);
         this->UpdateRateTrackBar->TabIndex = 3;
         // 
         // label2
         // 
         this->label2->AutoSize = true;
         this->label2->Location = System::Drawing::Point(12, 91);
         this->label2->Name = L"label2";
         this->label2->Size = System::Drawing::Size(68, 13);
         this->label2->TabIndex = 4;
         this->label2->Text = L"Update Rate";
         // 
         // label3
         // 
         this->label3->AutoSize = true;
         this->label3->Location = System::Drawing::Point(12, 48);
         this->label3->Name = L"label3";
         this->label3->Size = System::Drawing::Size(51, 13);
         this->label3->TabIndex = 5;
         this->label3->Text = L"Server IP";
         // 
         // ServerIPDropDown
         // 
         this->ServerIPDropDown->FormattingEnabled = true;
         this->ServerIPDropDown->Location = System::Drawing::Point(92, 45);
         this->ServerIPDropDown->Name = L"ServerIPDropDown";
         this->ServerIPDropDown->Size = System::Drawing::Size(240, 21);
         this->ServerIPDropDown->TabIndex = 6;
         // 
         // Form1
         // 
         this->AutoScaleDimensions = System::Drawing::SizeF(6, 13);
         this->AutoScaleMode = System::Windows::Forms::AutoScaleMode::Font;
         this->ClientSize = System::Drawing::Size(360, 219);
         this->Controls->Add(this->ServerIPDropDown);
         this->Controls->Add(this->label3);
         this->Controls->Add(this->label2);
         this->Controls->Add(this->UpdateRateTrackBar);
         this->Controls->Add(this->StartButton);
         this->Controls->Add(this->label1);
         this->Controls->Add(this->ConsoleNameDropdown);
         this->FormBorderStyle = System::Windows::Forms::FormBorderStyle::FixedSingle;
         this->MaximizeBox = false;
         this->Name = L"Form1";
         this->Text = L"screencap";
         (cli::safe_cast<System::ComponentModel::ISupportInitialize^  >(this->UpdateRateTrackBar))->EndInit();
         this->ResumeLayout(false);
         this->PerformLayout();

      }
#pragma endregion
   



      private: System::Void StartButton_Click(System::Object^  sender, System::EventArgs^  e)
      {
         sender;
         e;
         
         if (!BScreenCapServer::getInitialized())
            return;
         
         if (BScreenCapServer::getCapturing())
         {
            setWaiting();
            
            BScreenCapServer::endCapture();
            
            resetWaiting();
            
            StartButton->Text = "Start";
            ServerIPDropDown->Enabled = true;
            ConsoleNameDropdown->Enabled = true;
            UpdateRateTrackBar->Enabled = true;
         }
         else if (ServerIPDropDown->Text->Length)
         {
            setWaiting();  
                     
            const char* pStr = (char*)(void*)Marshal::StringToHGlobalAnsi(ServerIPDropDown->Text);
            
            BScreenCapServer::beginCapture(pStr, 1.0f);
            
            Marshal::FreeHGlobal((System::IntPtr)(void*)pStr);
            
            StartButton->Text = "Stop";
            ServerIPDropDown->Enabled = false;
            ConsoleNameDropdown->Enabled = false;
            UpdateRateTrackBar->Enabled = false;
            
            startXboxViewer();
            
            resetWaiting();
         }            
      }

};
}

