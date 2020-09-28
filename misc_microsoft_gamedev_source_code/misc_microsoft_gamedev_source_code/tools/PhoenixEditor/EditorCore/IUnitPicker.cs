using System;
using System.Collections.Generic;
using System.Text;

namespace EditorCore
{
   public interface IUnitPicker
   {
      void PlayerIdSelectedChanged(int index);
      void UnitSelectedChanged(object obj);
   }

   public interface ISquadPicker
   {
      void SquadSelectedChanged(ProtoSquadXml squad);
   }
}
